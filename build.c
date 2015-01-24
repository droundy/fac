#define _GNU_SOURCE

#include "bilge.h"
#include "lib/bigbrother.h"
#include "lib/listset.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

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

struct building {
  int all_done;
  pid_t pid;
  pid_t grandchild_pid;
  int stdouterrfd;
  double build_time;
  struct rule *rule;
  arrayset readdir, read, written, deleted;
};

static void *run_parallel_rule(void *void_building) {
  struct building *b = void_building;
  const char **args = malloc(4*sizeof(char *));
  initialize_arrayset(&b->readdir);
  initialize_arrayset(&b->read);
  initialize_arrayset(&b->written);
  initialize_arrayset(&b->deleted);
  b->all_done = dirty;

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
                                        &b->grandchild_pid,
                                        &b->readdir,
                                        &b->read, &b->written, &b->deleted);
  struct timeval stopped;
  gettimeofday(&stopped, 0);
  b->build_time = stopped.tv_sec - started.tv_sec + 1e-6*(stopped.tv_usec - started.tv_usec);
  if (ret != 0) {
    b->all_done = failed;
    return 0;
  }

  b->all_done = built;
  return 0;
}

static bool determine_rule_cleanliness(struct all_targets *all, struct rule *r,
                                       int *num_to_build) {
  assert(r);
  if (r->status == being_determined) {
    verbose_printf("Looks like a cycle! %s\n", r->outputs[0]->path);
  }
  if (r->status == unready) r->status = dirty; /* reset to dirty! */
  if (r->status != unknown) return false;
  r->status = being_determined;
  for (int i=0;i<r->num_inputs;i++) {
    if (r->inputs[i]->rule) {
      if (r->inputs[i]->rule->status == unknown)
        determine_rule_cleanliness(all, r->inputs[i]->rule, num_to_build);
      if (r->inputs[i]->rule->status == being_determined)
        verbose_printf("CYCLE INVOLVING %s and %s\n  and %s\n",
                       r->outputs[0]->path,
                       r->inputs[i]->path,
                       r->inputs[i]->rule->outputs[0]->path);
      if (r->inputs[i]->rule->status == dirty ||
          r->inputs[i]->rule->status == built ||
          r->inputs[i]->rule->status == building) {
        verbose_printf("::: %s :::\n", r->command);
        verbose_printf(" - dirty because %s needs to be rebuilt.\n",
                       r->inputs[i]->path);
        r->status = dirty;
        *num_to_build += 1;
        return true;
      }
    }
    if (r->input_times[i]) {
      if (!create_target_with_stat(all, r->inputs[i]->path) ||
          r->input_times[i] != r->inputs[i]->last_modified ||
          r->input_sizes[i] != r->inputs[i]->size) {
        verbose_printf("::: %s :::\n", r->command);
        verbose_printf(" - dirty because %s has wrong input time.\n",
               r->inputs[i]->path);
        r->status = dirty;
        *num_to_build += 1;
        return true; /* The file is out of date. */
      }
    } else {
      verbose_printf("::: %s :::\n", r->command);
      verbose_printf(" - dirty because #%d %s has no input time.\n", i, r->inputs[i]->path);
      r->status = dirty;
      *num_to_build += 1;
      return true; /* The file hasn't been built. */
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
        /* printf("   compare times %ld with %ld\n", */
        /*        r->outputs[i]->last_modified, r->output_times[i]); */
        r->status = dirty;
        *num_to_build += 1;
        return true; /* The file is out of date. */
      }
    } else {
      verbose_printf("::: %s :::\n", r->command);
      verbose_printf(" - dirty because %s has no output time.\n", r->outputs[i]->path);
      r->status = dirty;
      *num_to_build += 1;
      return true; /* The file hasn't been built. */
    }
  }
  r->status = clean;
  return false;
}

static void find_latency(struct rule *r) {
  if (!r) return;
  if (r->status != dirty) return;
  if (r->latency_handled) return;
  r->latency_handled = true;
  double maxchild = 0;
  for (int i=0;i<r->num_outputs;i++) {
    find_latency(r->outputs[i]->rule);
    if (r->outputs[i]->rule->latency_estimate > maxchild)
      maxchild = r->outputs[i]->rule->latency_estimate;
  }
  r->latency_estimate = r->build_time + maxchild;
}

static struct building *build_rule_or_dependency(struct all_targets *all,
                                                 struct rule *r,
                                                 int *num_to_build) {
  assert(r);
  if (r->status == unready) return 0; /* we already tried this */
  if (r->status == unknown) {
    determine_rule_cleanliness(all, r, num_to_build);
  }
  if (r->status == failed) {
    verbose_printf("already failed once: %s\n", r->command);
    return 0;
  }
  if (r->status == dirty) {
    r->status = unready; /* We will change this if we start building */
    bool ready_to_go = true;
    for (int i=0;i<r->num_inputs;i++) {
      if (r->inputs[i]->rule && r->inputs[i]->rule->status == failed) {
        r->status = failed;
        return 0;
      }
      if (r->inputs[i]->rule && !(r->inputs[i]->rule->status == clean || r->inputs[i]->rule->status == built)) {
        struct building *b = build_rule_or_dependency(all, r->inputs[i]->rule,
                                                      num_to_build);
        if (b) return b;
        ready_to_go = false;
      }
    }

    if (ready_to_go) {
      //struct building *b = malloc(sizeof(struct building));
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
      return b;
    }
  }
  return 0;
}

void let_us_build(struct all_targets *all, struct rule *r, int *num_to_build,
                  struct building **bs, int num_threads) {
  for (int i=0;i<num_threads;i++) {
    if (!bs[i]) {
      bs[i] = build_rule_or_dependency(all, r, num_to_build);
      if (bs[i]) {
        pid_t new_pid = fork();
        if (new_pid == 0) {
          run_parallel_rule(bs[i]);

          /* The following is extremely hokey and stupid.  Rather
             than exiting the forked process, we exec /bin/true
             (which has the same effect, but less efficiently) so
             that profilers won't get confused by us forking but not
             calling exec. */
          char **args = malloc(2*sizeof(char *));
          args[0] = "/bin/true";
          args[1] = 0;
          execv("/bin/true", args);
          exit(0);
        }
        bs[i]->pid = new_pid;
      }
    }
  }
}

int num_jobs = 0;

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

void parallel_build_all(struct all_targets *all, const char *root_, bool bilgefiles_only) {
  root = root_;
  git_files_content = git_ls_files();
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

  int num_to_build = 0, num_built = 0, num_failed = 0, num_to_go = 0;

  double total_cpu_time_spent = 0;
  bool have_finished_building_and_reading_bilges = false;
  bool have_checked_for_impossibilities = false;
  struct rule_list *rules = 0;
  listset *bilgefiles_used = 0;
  do {
    bool newstufftobuild = false;
    for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
      newstufftobuild |= determine_rule_cleanliness(all, r, &num_to_build);
    }
    if (newstufftobuild) {
      delete_rule_list(&rules);
      for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
        find_latency(r);
        /* sadly, the following is O(N^2) */
        if (r->status != clean) insert_rule_by_latency(&rules, r);
      }
    }
    double total_build_time = 0;
    for (struct rule_list *rr = rules; rr; rr = rr->next) {
      if (rr->r->status == dirty || rr->r->status == building)
        total_build_time += rr->r->build_time;
    }
    if (total_build_time/num_jobs > 1.0) {
      int build_minutes = (int)(total_build_time/num_jobs/60);
      double build_seconds = total_build_time/num_jobs - 60*build_minutes;
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

    int threads_in_use = 0;
    for (int i=0;i<num_jobs;i++) {
      if (bs[i]) threads_in_use++;
    }

    if (threads_in_use) {
      pid_t pid = waitpid(-1, 0, 0);
      for (int i=0;i<num_jobs;i++) {
        if (bs[i] && bs[i]->pid == pid) {
          bs[i]->rule->build_time = bs[i]->build_time;
          total_cpu_time_spent += bs[i]->build_time;
          bs[i]->rule->status = bs[i]->all_done;
          printf("%d/%d [%.2fs]: %s\n", num_built+1, num_to_build,
                 bs[i]->rule->build_time, bs[i]->rule->command);
          off_t stdoutlen = lseek(bs[i]->stdouterrfd, 0, SEEK_END);
          off_t myoffset = 0;
          if (bs[i]->all_done != built || show_output)
            sendfile(1, bs[i]->stdouterrfd, &myoffset, stdoutlen);
          close(bs[i]->stdouterrfd);

          if (bs[i]->all_done == built) {
            struct rule *r = bs[i]->rule;
            delete_rule(&rules, r);
            insert_to_listset(&bilgefiles_used, r->bilgefile_path);

            /* FIXME We should verify that the inputs specified were actually used */

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
                r->status = failed;
                num_failed++;
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

            num_built++;
          } else if (bs[i]->all_done == failed) {
            if (bs[i]->rule->num_outputs == 1) {
              printf("build failed: %s\n",
                     pretty_path(bs[i]->rule->outputs[0]->path));
            } else {
              printf("build failed: %s\n", bs[i]->rule->command);
            }
            num_failed++;
          } else {
            printf("INTERRUPTED!\n");
            bs[i]->rule->status = failed;
            am_interrupted = true;
            num_failed++;
          }
          munmap(bs[i], sizeof(struct building));
          bs[i] = 0;
        }
      }
    }

    if (!have_finished_building_and_reading_bilges) {
      have_finished_building_and_reading_bilges = true;
      for (struct target *t = (struct target *)all->t.first; t; t = (struct target *)t->e.next) {
        int len = strlen(t->path);
        if (len >= 6 && !strcmp(t->path+len-6, ".bilge")) {
          if (t->status == unknown &&
              (!t->rule || (t->rule->status != dirty && t->rule->status != building))) {
            /* This is a clean .bilge file, but we still need to parse it! */
            read_bilge_file(all, t->path);
            have_finished_building_and_reading_bilges = false;
            t->status = built;
          }
        }
      }
      for (struct target *t = (struct target *)all->t.first; t; t = (struct target *)t->e.next) {
        int len = strlen(t->path);
        if (len >= 6 && !strcmp(t->path+len-6, ".bilge")) {
          if (t->rule && t->rule->status == dirty) {
            /* This is a dirty .bilge file, so we need to build it! */
            let_us_build(all, t->rule, &num_to_build, bs, num_jobs);
            have_finished_building_and_reading_bilges = false;
          }
        }
      }
      if (!have_finished_building_and_reading_bilges) continue;
    }

    if (!have_checked_for_impossibilities) {
      if (bilgefiles_only) {
        /* We are only building and parsing the bilgefiles. */
        delete_rule_list(&rules);
        return;
      }
      for (struct rule_list *rr = rules; rr; rr = rr->next) {
        if (rr->r->status == dirty || rr->r->status == unknown) {
          for (int i=0;i<rr->r->num_inputs;i++) {
            if (!rr->r->inputs[i]->rule && i < rr->r->num_explicit_inputs) {
              if (access(rr->r->inputs[i]->path, R_OK)) {
                printf("cannot build: %s due to missing input %s\n",
                       pretty_path(rr->r->outputs[0]->path),
                       pretty_path(rr->r->inputs[i]->path));
                rr->r->status = failed;
                num_failed++;
              }
              const char *p = rr->r->inputs[i]->path;
              if (p != pretty_path(p) && git_files_content &&
                  !is_in_git(rr->r->inputs[i]->path)) {
                printf("error: %s should be in git\n",
                       pretty_path(rr->r->inputs[i]->path));
                num_failed++;
              }
              //XXX add to let_us_build a repeat check... or above anyhow
            }
          }
        }
      }
      have_checked_for_impossibilities = true;
    }

    num_to_go = 0;
    for (struct rule_list *rr = rules; rr; rr = rr->next) {
      if (rr->r->status == dirty || rr->r->status == building) {
        let_us_build(all, rr->r, &num_to_build, bs, num_jobs);
        //printf("still need to build %s\n", rr->r->command);
        num_to_go++;
      }
    }
    if (am_interrupted) {
      for (int i=0;i<num_jobs;i++) {
        if (bs[i]) {
          kill(bs[i]->grandchild_pid, SIGTERM);
          kill(bs[i]->pid, SIGTERM);
          munmap(bs[i], sizeof(struct building));
          bs[i] = 0;
        }
      }
      printf("Interrupted!                    \n");

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
  } while (num_to_go || !have_finished_building_and_reading_bilges);

  while (bilgefiles_used) {
    char *donefile = done_name(bilgefiles_used->path);
    FILE *f = fopen(donefile, "w");
    if (!f) error(1,errno,"oopse");
    fprint_bilgefile(f, all, bilgefiles_used->path);
    fclose(f);
    free(donefile);
    bilgefiles_used = bilgefiles_used->next;
  }

  delete_rule_list(&rules);
  if (num_failed) {
    printf("Failed %d/%d builds, succeeded %d/%d builds\n", num_failed, num_to_build, num_built, num_to_build);
    exit(1);
  } else {
    find_elapsed_time();
    printf("Build succeeded! %.0f:%05.2f      \n",
           elapsed_minutes, elapsed_seconds);
  }
  free(bs);
  sigaction(SIGINT, &oldact, 0);
}

void clean_all(struct all_targets *all, const char *root_) {
  root = root_;
  for (struct target *t = (struct target *)all->t.first; t; t = (struct target *)t->e.next) {
    t->status = unknown;
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
