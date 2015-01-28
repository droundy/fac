#define _GNU_SOURCE

#include "bilge.h"
#include "new-build.h"

#include "lib/bigbrother.h"
#include "lib/listset.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/sendfile.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

static inline bool is_bilgefile(const char *path) {
  int len = strlen(path);
  return len >= 6 && !strcmp(path+len-6, ".bilge");
}

bool is_interesting_path(struct rule *r, const char *path) {
  const int len = strlen(path);
  for (int i=0;i<r->num_cache_prefixes;i++) {
    bool matches = true;
    for (int j=0;r->cache_prefixes[i][j];j++) {
      if (path[j] != r->cache_prefixes[i][j]) {
        matches = false;
        break;
      }
    }
    if (matches) return false;
  }
  for (int i=0;i<r->num_cache_suffixes;i++) {
    bool matches = true;
    for (int j=0;r->cache_suffixes_reversed[i][j];j++) {
      if (path[len-j-1] != r->cache_suffixes_reversed[i][j]) {
        matches = false;
        break;
      }
    }
    if (matches) return false;
  }
  return true;
}

static void check_cleanliness(struct all_targets *all, struct rule *r);

void mark_rule(struct all_targets *all, struct rule *r) {
  if (r->status != unknown) {
    printf("command status %d %s\n", r->status, r->command);
    return;
  }
  r->status = marked;
  put_rule_into_status_list(&all->marked_list, r);
}

void mark_bilgefiles(struct all_targets *all) {
  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    if (r->status == unknown) {
      for (int i=0;i<r->num_outputs;i++) {
        if (is_bilgefile(r->outputs[i]->path)) mark_rule(all, r);
      }
    }
  }
}
void mark_all(struct all_targets *all) {
  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    if (r->status == unknown) mark_rule(all, r);
  }
}

void rule_is_clean(struct all_targets *all, struct rule *r) {
  r->status = clean;
  put_rule_into_status_list(&all->clean_list, r);
}
static void find_latency(struct rule *r) {
  if (r->latency_handled) return;
  r->latency_handled = true;
  double maxchild = 0;
  for (int i=0;i<r->num_outputs;i++) {
    for (int j=0;j<r->outputs[i]->num_children;j++) {
      find_latency(r->outputs[i]->children[j]);
      if (r->outputs[i]->children[j]->latency_estimate > maxchild)
        maxchild = r->outputs[i]->children[j]->latency_estimate;
    }
  }
  r->latency_estimate = r->build_time + maxchild;
}
void rule_is_ready(struct all_targets *all, struct rule *r) {
  if (r->status == unready) all->unready_num--;
  else all->estimated_time += r->build_time/num_jobs;
  all->ready_num++;
  r->status = dirty;

  /* remove from its former list */
  if (r->status_prev) {
    (*r->status_prev) = r->status_next;
  }
  if (r->status_next) {
    r->status_next->status_prev = r->status_prev;
  }

  /* the following keeps the read_list sorted by latency */

  /* Note that this costs O(N) in the number of ready jobs, and thus
     could be very expensive.  So we probably should throttle this
     feature based on the number of ready jobs. */
  find_latency(r);
  struct rule **list = &all->ready_list;
  while (*list && (*list)->latency_estimate > r->latency_estimate) {
    list = &(*list)->status_next;
  }
  r->status_next = *list;
  r->status_prev = list;
  if (r->status_next) r->status_next->status_prev = &r->status_next;
  *list = r;
}
void built_rule(struct all_targets *all, struct rule *r) {
  r->status = built;
  all->estimated_time -= r->old_build_time/num_jobs;
  all->ready_num--;
  all->built_num++;
  put_rule_into_status_list(&all->clean_list, r);
  for (int i=0;i<r->num_outputs;i++) {
    for (int j=0;j<r->outputs[i]->num_children;j++) {
      check_cleanliness(all, r->outputs[i]->children[j]);
    }
  }
}
void rule_failed(struct all_targets *all, struct rule *r) {
  if (r->status == failed) return; /* just in case! */
  all->estimated_time -= r->old_build_time/num_jobs;
  if (r->status == unready) {
    all->unready_num--;
  } else {
    all->ready_num--;
  }
  all->failed_num++;
  r->status = failed;
  put_rule_into_status_list(&all->clean_list, r);
  for (int i=0;i<r->num_outputs;i++) {
    for (int j=0;j<r->outputs[i]->num_children;j++) {
      rule_failed(all, r->outputs[i]->children[j]);
    }
  }
}
void rule_is_unready(struct all_targets *all, struct rule *r) {
  r->status = unready;
  all->unready_num++;
  all->estimated_time += r->build_time/num_jobs;
  put_rule_into_status_list(&all->unready_list, r);
}

static struct target *create_target_with_stat(struct all_targets *all,
                                              const char *path) {
  struct target *t = create_target(all, path);
  if (!t->last_modified) {
    struct stat st;
    if (stat(t->path, &st)) return 0;
    t->size = st.st_size;
    t->last_modified = st.st_mtime;
  }
  return t;
}

void check_cleanliness(struct all_targets *all, struct rule *r) {
  if (r->status != unknown && r->status != unready && r->status != marked) {
    return;
  }
  int old_status = r->status;
  r->status = being_determined;
  for (int i=0;i<r->num_inputs;i++) {
    if (r->inputs[i]->rule) {
      if (r->inputs[i]->rule->status == unknown ||
          r->inputs[i]->rule->status == marked) {
        check_cleanliness(all, r->inputs[i]->rule);
      }
      if (r->inputs[i]->rule->status == being_determined)
        error_at_line(1, 0, r->bilgefile_path, r->bilgefile_linenum,
                      "CYCLE INVOLVING %s and %s\n  and %s\n",
                      r->outputs[0]->path,
                      r->inputs[i]->path,
                      r->inputs[i]->rule->outputs[0]->path);
      if (r->inputs[i]->rule->status == dirty ||
          r->inputs[i]->rule->status == unready ||
          r->inputs[i]->rule->status == building) {
        r->status = unready;
        if (old_status == unready) {
          r->status = unready;
          return; /* no change so we can just set status back to what it was. */
        }
        verbose_printf("::: %s :::\n", r->command);
        verbose_printf(" - dirty because %s needs to be rebuilt.\n",
                       r->inputs[i]->path);
        rule_is_unready(all, r);
        return;
      }
    }
  }
  if (old_status == unready) {
    r->status = old_status;
    rule_is_ready(all, r);
    /* FIXME if we get to the point of checking output content, then
       we will want to change this bit so sometimes it will result in
       a clean state. */
    return;
  }
  for (int i=0;i<r->num_inputs;i++) {
    if (r->inputs[i]->rule && r->inputs[i]->rule->status == built) {
      verbose_printf("::: %s :::\n", r->command);
      verbose_printf(" - dirty because %s has been rebuilt.\n",
                     r->inputs[i]->path);
      rule_is_ready(all, r);
      return;
    }
    if (r->input_times[i]) {
      if (!create_target_with_stat(all, r->inputs[i]->path) ||
          r->input_times[i] != r->inputs[i]->last_modified ||
          r->input_sizes[i] != r->inputs[i]->size) {
        verbose_printf("::: %s :::\n", r->command);
        verbose_printf(" - dirty because %s has wrong input time.\n",
               r->inputs[i]->path);
        rule_is_ready(all, r);
        return;
      }
    } else {
      verbose_printf("::: %s :::\n", r->command);
      verbose_printf(" - dirty because #%d %s has no input time.\n", i, r->inputs[i]->path);
      rule_is_ready(all, r);
      return;
    }
  }
  for (int i=0;i<r->num_outputs;i++) {
    if (r->output_times[i]) {
      if (!create_target_with_stat(all, r->outputs[i]->path) ||
          r->output_times[i] != r->outputs[i]->last_modified ||
          r->output_sizes[i] != r->outputs[i]->size) {
        verbose_printf("::: %s :::\n", r->command);
        verbose_printf(" - dirty because %s has wrong output time.\n",
               r->outputs[i]->path);
        rule_is_ready(all, r);
        return;
      }
    } else {
      verbose_printf("::: %s :::\n", r->command);
      verbose_printf(" - dirty because %s has no output time.\n", r->outputs[i]->path);
      rule_is_ready(all, r);
      return;
    }
  }
  rule_is_clean(all, r);
}

static const char *root = 0;
static const char *pretty_path(const char *path) {
  int len = strlen(root);
  if (path[len] == '/' && !memcmp(path, root, len)) {
    return path + len + 1;
  }
  return path;
}

static struct trie *git_files_content = 0;
static bool is_in_git(const char *path) {
  return lookup_in_trie(&git_files_content, pretty_path(path)) != 0;
}

struct building {
  int all_done;
  pid_t pid;
  int stdouterrfd;
  double build_time;
  struct rule *rule;
  arrayset readdir, read, written, deleted;
};

void remove_from_status_list(struct rule *r) {
  if (r->status_prev) {
    *(r->status_prev) = r->status_next;
  }
  if (r->status_next) {
    r->status_next->status_prev = r->status_prev;
  }
  r->status_prev = 0;
  r->status_next = 0;
}

static struct building *build_rule(struct all_targets *all,
                                   struct rule *r) {
  assert(r);
  struct building *b = mmap(NULL, sizeof(struct building),
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  b->rule = r;
  const char *templ = "/tmp/bilge-XXXXXX";
  char *namebuf = malloc(strlen(templ)+1);
  strcpy(namebuf, templ);
  b->stdouterrfd = mkstemp(namebuf);
  unlink(namebuf);
  free(namebuf);
  b->all_done = building;
  r->status = building;
  put_rule_into_status_list(&all->running_list, r);
  return b;
}

void let_us_build(struct all_targets *all, struct rule *r,
                  struct building **bs) {
  for (int i=0;i<num_jobs;i++) {
    if (!bs[i]) {
      bs[i] = build_rule(all, r);
      pid_t new_pid = fork();
      if (new_pid == 0) {
        struct building *b = bs[i];
        const char **args = malloc(4*sizeof(char *));
        initialize_arrayset(&b->readdir);
        initialize_arrayset(&b->read);
        initialize_arrayset(&b->written);
        initialize_arrayset(&b->deleted);
        b->all_done = dirty;

        setpgid(0,0); // causes children to be killed along with this
        close(1);
        close(2);
        dup(b->stdouterrfd);
        dup(b->stdouterrfd);

        args[0] = "/bin/sh";
        args[1] = "-c";
        args[2] = b->rule->command;
        args[3] = 0;

        struct timeval started;
        gettimeofday(&started, 0);
        int ret = bigbrother_process_arrayset(b->rule->working_directory,
                                              (char **)args,
                                              &b->readdir,
                                              &b->read, &b->written, &b->deleted);
        struct timeval stopped;
        gettimeofday(&stopped, 0);
        b->build_time = stopped.tv_sec - started.tv_sec + 1e-6*(stopped.tv_usec - started.tv_usec);
        if (ret != 0) {
          b->all_done = failed;
        } else {
          b->all_done = built;
        }

        /* The following is extremely hokey and stupid.  Rather
           than exiting the forked process, we exec /bin/true
           (which has the same effect, but less efficiently) so
           that profilers won't get confused by us forking but not
           calling exec. */
        args = malloc(2*sizeof(char *));
        args[0] = "/bin/true";
        args[1] = 0;
        execv("/bin/true", (char **)args);
        exit(0);
      }
      bs[i]->pid = new_pid;
      return;
    }
  }
}

static struct timeval starting;
static double elapsed_seconds, elapsed_minutes;
static bool am_interrupted = false;

static void handle_interrupt(int sig) {
  am_interrupted = true;
}

static void find_elapsed_time() {
  struct timeval now;
  gettimeofday(&now, 0);
  elapsed_seconds = ((now.tv_sec - starting.tv_sec) % 60) + (now.tv_usec - starting.tv_usec)*1e-6;
  elapsed_minutes = (now.tv_sec - starting.tv_sec) / 60;
  if (elapsed_seconds < 0) {
    elapsed_seconds += 60;
    elapsed_minutes -= 1;
  }
}

void check_for_impossibilities(struct all_targets *all, const char *_root) {
  root = _root;
  for (struct rule *r = all->marked_list; r; r = all->marked_list) {
    check_cleanliness(all, r);
  }
  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    if (r->status == dirty || r->status == unready) {
      for (int i=0;i<r->num_inputs;i++) {
        if (!r->inputs[i]->rule && i < r->num_explicit_inputs) {
          if (access(r->inputs[i]->path, R_OK)) {
            printf("cannot build: %s due to missing input %s\n",
                   pretty_path(r->outputs[0]->path),
                   pretty_path(r->inputs[i]->path));
            rule_failed(all, r);
          }
          const char *p = r->inputs[i]->path;
          if (p != pretty_path(p) && git_files_content &&
              !is_in_git(r->inputs[i]->path)) {
            printf("error: %s should be in git\n",
                   pretty_path(r->inputs[i]->path));
            rule_failed(all, r);
          }
        }
      }
    }
  }
}

void build_continual(const char *root_) {
  root = root_;
  while (!am_interrupted) {
    struct all_targets all;
    init_hash_table(&all.r, 1000);
    init_hash_table(&all.t, 10000);
    all.ready_list = all.unready_list = all.clean_list = all.failed_list = all.marked_list = 0;
    all.running_list = 0;
    all.ready_num = all.unready_num = all.failed_num = all.built_num = 0;
    all.estimated_time = 0;
    create_target(&all, "top.bilge");

    bool still_reading;
    do {
      still_reading = false;
      for (struct target *t = (struct target *)all.t.first; t; t = (struct target *)t->e.next) {
        if (t->status == unknown &&
            (!t->rule ||
             t->rule->status == clean ||
             t->rule->status == built)) {
          t->status = built;
          if (is_bilgefile(t->path)) {
            still_reading = true;
            read_bilge_file(&all, t->path);
          }
        }
      }
      build_marked(&all, root);
      for (struct target *t = (struct target *)all.t.first; t; t = (struct target *)t->e.next) {
        if (t->status == unknown &&
            (!t->rule ||
             t->rule->status == clean ||
             t->rule->status == built)) {
          t->status = built;
          if (is_bilgefile(t->path)) {
            still_reading = true;
            read_bilge_file(&all, t->path);
          }
        }
      }
      mark_bilgefiles(&all);
    } while (all.marked_list || still_reading);
    mark_all(&all);
    check_for_impossibilities(&all, root);
    build_marked(&all, root);
    summarize_build_results(&all);

    int ifd = inotify_init1(IN_CLOEXEC);
    for (struct target *t = (struct target *)all.t.first; t; t = (struct target *)t->e.next) {
      t->status = unknown;
      if (t->num_children || is_bilgefile(t->path)) {
        inotify_add_watch(ifd, t->path, IN_CLOSE_WRITE | IN_DELETE_SELF);
      }
    }
    for (struct rule *r = (struct rule *)all.r.first; r; r = (struct rule *)r->e.next) {
      r->status = unknown;
    }
    /* this is extremely sloppy */
    char *buffer = malloc(sizeof(struct inotify_event) + NAME_MAX + 1);
    read(ifd, buffer, sizeof(struct inotify_event) + NAME_MAX + 1);
    free(buffer);
    close(ifd);
  }
}

void build_marked(struct all_targets *all, const char *root_) {
  if (!root) root = root_;
  if (!git_files_content) git_files_content = git_ls_files();
  if (!all->marked_list && !all->ready_list) {
    if (all->failed_num) {
      printf("Failed to build %d files.\n", all->failed_num);
      exit(1);
    }
    return; /* nothing to build */
  }
  gettimeofday(&starting, 0);

  if (!num_jobs) num_jobs = sysconf(_SC_NPROCESSORS_ONLN);
  verbose_printf("Using %d jobs\n", num_jobs);

  struct building **bs = malloc(num_jobs*sizeof(struct building *));
  for (int i=0;i<num_jobs;i++) bs[i] = 0;
  struct sigaction act, oldact;
  act.sa_handler = handle_interrupt;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGINT, &act, &oldact);

  for (struct rule *r = all->marked_list; r; r = all->marked_list) {
    check_cleanliness(all, r);
  }

  listset *bilgefiles_used = 0;
  do {
    int threads_in_use = 0;
    for (int i=0;i<num_jobs;i++) {
      if (bs[i]) threads_in_use++;
    }

    if (threads_in_use) {
      pid_t pid = waitpid(-1, 0, 0);
      for (int i=0;i<num_jobs;i++) {
        if (bs[i] && bs[i]->pid == pid) {
          threads_in_use--;
          bs[i]->rule->old_build_time = bs[i]->rule->build_time;
          bs[i]->rule->build_time = bs[i]->build_time;
          printf("%d/%d [%.2fs]: %s\n",
                 1 + all->failed_num + all->built_num,
                 all->failed_num + all->built_num + all->ready_num + all->unready_num,
                 bs[i]->build_time, bs[i]->rule->command);
          if (all->estimated_time > 1.0) {
            int build_minutes = (int)(all->estimated_time/60);
            double build_seconds = all->estimated_time - 60*build_minutes;
            find_elapsed_time();
            double total_seconds = elapsed_seconds + build_seconds;
            double total_minutes = elapsed_minutes + build_minutes;
            if (total_seconds > 60) {
              total_minutes += 1;
              total_seconds -= 60;
            }
            printf("Build time remaining: %d:%02.0f / %.0f:%02.0f      \r",
                   build_minutes, build_seconds, total_minutes, total_seconds);
            fflush(stdout);
          }
          if (bs[i]->all_done != built || show_output) {
            off_t stdoutlen = lseek(bs[i]->stdouterrfd, 0, SEEK_END);
            off_t myoffset = 0;
            sendfile(1, bs[i]->stdouterrfd, &myoffset, stdoutlen);
          }
          if (bs[i]->all_done != built && bs[i]->all_done != failed) {
            printf("INTERRUPTED!\n");
            am_interrupted = true;
            break;
          }

          if (bs[i]->all_done == failed) {
            rule_failed(all, bs[i]->rule);
            if (bs[i]->rule->num_outputs == 1) {
              printf("build failed: %s\n",
                     pretty_path(bs[i]->rule->outputs[0]->path));
            } else {
              printf("build failed: %s\n", bs[i]->rule->command);
            }
            break;
          }
          close(bs[i]->stdouterrfd);

          struct rule *r = bs[i]->rule;
          insert_to_listset(&bilgefiles_used, r->bilgefile_path);

          /* Forget the non-explicit imputs, as we will re-add those
             inputs that were actually used in the build */
          r->num_inputs = r->num_explicit_inputs;
          for (int ii=0;ii<r->num_inputs;ii++) {
            struct target *t = create_target_with_stat(all, r->inputs[ii]->path);
            if (!t) {
              error(1, 0, "Unable to stat input file %s (this should be impossible)\n",
                    r->inputs[ii]->path);
            } else {
              add_input(r, t);
              delete_from_arrayset(&bs[i]->read, r->inputs[ii]->path);
              delete_from_arrayset(&bs[i]->written, r->inputs[ii]->path);
            }
          }
          /* Forget the non-explicit outputs, as we will re-add
             those inputs that were actually created in the build */
          r->num_outputs = r->num_explicit_outputs;
          for (int ii=0;ii<r->num_outputs;ii++) {
            struct target *t = lookup_target(all, r->outputs[ii]->path);
            if (t) {
              t->last_modified = 0;
              t->size = 0;
            }
            t = create_target_with_stat(all, r->outputs[ii]->path);
            if (!t) {
              printf("build failed to create: %s\n",
                     pretty_path(r->outputs[ii]->path));
              rule_failed(all, r);
              break;
            } else {
              t->rule = r;
              add_output(r, t);
              delete_from_arrayset(&bs[i]->read, r->outputs[ii]->path);
              delete_from_arrayset(&bs[i]->written, r->outputs[ii]->path);
            }
          }
          for (char *path = start_iterating(&bs[i]->read); path; path = iterate(&bs[i]->read)) {
            if (is_interesting_path(r, path)) {
              struct target *t = create_target_with_stat(all, path);
              if (!t) error(1, errno, "Unable to input stat file %s", path);
              add_input(r, t);
            }
          }

          for (char *path = start_iterating(&bs[i]->readdir); path; path = iterate(&bs[i]->readdir)) {
            if (is_interesting_path(r, path)) {
              struct target *t = create_target_with_stat(all, path);
              if (!t) error(1, errno, "Unable to stat directory %s", path);
              add_input(r, t);
            }
          }

          for (char *path = start_iterating(&bs[i]->written); path; path = iterate(&bs[i]->written)) {
            if (is_interesting_path(r, path)) {
              struct target *t = lookup_target(all, path);
              if (t) {
                t->last_modified = 0;
                t->size = 0;
              }
              t = create_target_with_stat(all, path);
              if (t) {
                if (path == pretty_path(path))
                  error(1,0,"Command created file outside source directory: %s\n| %s",
                        path, r->command);
                if (t->rule && t->rule != r)
                  error(1,0,"Two rules generate same output: %s\n| %s\n| %s",
                        pretty_path(path), r->command, t->rule->command);
                t->rule = r;
                add_output(r, t);
              }
            }
          }
          built_rule(all, r);
          insert_to_listset(&bilgefiles_used, r->bilgefile_path);

          munmap(bs[i], sizeof(struct building));
          bs[i] = 0;
        }
      }
    }

    {
      int N = num_jobs - threads_in_use;
      struct rule **toqueue = calloc(N, sizeof(struct rule *));
      int i = 0;
      for (struct rule *r = all->ready_list; r && i < N; r = r->status_next) {
        toqueue[i++] = r;
      }
      for (i=0;i<N;i++) {
        if (toqueue[i]) let_us_build(all, toqueue[i], bs);
      }
      free(toqueue);
    }
    if (am_interrupted) {
      for (int i=0;i<num_jobs;i++) {
        if (bs[i]) {
          verbose_printf("killing %d (%s)\n", bs[i]->pid, bs[i]->rule->command);
          kill(-bs[i]->pid, SIGTERM); /* ask child to die */
        }
      }
      sleep(1); /* give them a second to die politely */
      for (int i=0;i<num_jobs;i++) {
        if (bs[i]) {
          kill(-bs[i]->pid, SIGKILL); /* now kill with extreme prejudice */
          munmap(bs[i], sizeof(struct building));
          bs[i] = 0;
        }
      }
      printf("Interrupted!                         \n");

      while (bilgefiles_used) {
        char *donefile = done_name(bilgefiles_used->path);
        FILE *f = fopen(donefile, "w");
        if (!f) error(1,errno,"oopse");
        fprint_bilgefile(f, all, bilgefiles_used->path);
        fclose(f);
        free(donefile);
        bilgefiles_used = bilgefiles_used->next;
      }

      exit(1);
    }
  } while (all->ready_list || all->running_list);

  while (bilgefiles_used) {
    char *donefile = done_name(bilgefiles_used->path);
    FILE *f = fopen(donefile, "w");
    if (!f) error(1,errno,"oopse");
    fprint_bilgefile(f, all, bilgefiles_used->path);
    fclose(f);
    free(donefile);
    bilgefiles_used = bilgefiles_used->next;
  }

  free(bs);
  sigaction(SIGINT, &oldact, 0);
}

void summarize_build_results(struct all_targets *all) {
  if (all->failed_list || all->failed_num) {
    printf("Build failed %d/%d failures\n", all->failed_num,
           all->failed_num + all->built_num);
    exit(1);
  } else {
    find_elapsed_time();
    printf("Build succeeded! %.0f:%05.2f      \n",
           elapsed_minutes, elapsed_seconds);
  }
}

void clean_all(struct all_targets *all, const char *root_) {
  root = root_;
  for (struct target *t = (struct target *)all->t.first; t; t = (struct target *)t->e.next) {
    t->status = unknown;
    int len = strlen(t->path);
    if (len >= 6 && !strcmp(t->path+len-6, ".bilge")) {
      char *donef = malloc(len + 20);
      strcpy(donef, t->path);
      strcat(donef, ".done");
      printf("rm %s\n", donef);
      unlink(donef);
      free(donef);
    }
  }
  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    for (int i=0;i<r->num_outputs;i++) {
      if (r->outputs[i]->status == unknown) {
        printf("rm %s\n", pretty_path(r->outputs[i]->path));
        unlink(r->outputs[i]->path);
      }
    }
  }
}
