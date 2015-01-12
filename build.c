#define _GNU_SOURCE

#include "bilge.h"
#include "lib/bigbrother.h"

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

static struct target *create_target_with_stat(struct all_targets **all,
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
  clock_t build_time;
  clock_t overhead_time;
  struct rule *rule;
  arrayset readdir, read, written, deleted;
};

void *run_parallel_rule(void *void_building) {
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

  int ret = bigbrother_process_arrayset(b->rule->working_directory,
                                        (char **)args,
                                        &b->grandchild_pid,
                                        &b->readdir,
                                        &b->read, &b->written, &b->deleted);
  struct tms thetimes;
  if (times(&thetimes) != -1) {
    b->build_time = thetimes.tms_utime + thetimes.tms_stime +
      thetimes.tms_cutime + thetimes.tms_cstime;
    b->overhead_time = thetimes.tms_utime + thetimes.tms_stime;
  } else {
    b->build_time = 0;
    b->overhead_time = 0;
  }
  if (ret != 0) {
    printf("XX FAILED %s\n", b->rule->command);
    b->all_done = failed;
    return 0;
  }

  b->all_done = built;
  return 0;
}

struct rule *run_rule(struct all_targets **all, struct rule *r) {
  struct rule *out = r; //create_rule(r->command, r->working_directory);
  listset *read_set = 0, *written_set = 0, *deleted_set = 0;
  listset *readdir_set = 0;
  const char **args = malloc(4*sizeof(char *));
  args[0] = "/bin/sh";
  args[1] = "-c";
  args[2] = r->command;
  args[3] = 0;
  printf("%s\n", r->command);
  int ret = bigbrother_process(r->working_directory,
                               (char **)args, &readdir_set,
                               &read_set, &written_set, &deleted_set);
  free(args);
  if (ret != 0) {
    free_listset(read_set);
    free_listset(written_set);
    free_listset(deleted_set);
    free_listset(readdir_set);
    return 0;
  }

  out->num_inputs = 0; // clear the set of inputs so we only rebuild on actual ones.
  listset *s = read_set;
  while (s != NULL) {
    struct target *t = create_target_with_stat(all, s->path);
    if (!t) error(1, errno, "Unable to stat file %s", t->path);
    add_input(out, t);
    s = s->next;
  }

  s = readdir_set;
  while (s != NULL) {
    printf("READDIR %s\n", s->path);
    struct target *t = create_target_with_stat(all, s->path);
    if (!t) error(1, errno, "Unable to stat file %s", t->path);
    add_input(out, t);
    s = s->next;
  }

  for (int i=0;i<out->num_outputs;i++) {
    /* The following handles the case where we have a command that
       doesn't actually write to one of its "outputs." */
    create_target_with_stat(all, out->outputs[i]->path);
  }

  s = written_set;
  while (s != NULL) {
    struct target *t = lookup_target(*all, s->path);
    if (t) {
      t->last_modified = 0;
      t->size = 0;
    }
    t = create_target_with_stat(all, s->path);
    if (!t) error(1, errno, "Unable to stat file %s", t->path);
    t->rule = out;
    add_output(out, t);
    s = s->next;
  }

  free_listset(readdir_set);
  free_listset(read_set);
  free_listset(written_set);
  free_listset(deleted_set);
  return out;
}

bool determine_rule_cleanliness(struct all_targets **all, struct rule *r,
                                int *num_to_build) {
  if (!r) return false;
  if (r->status == being_determined) {
    verbose_printf("Looks like a cycle! %s\n",
                   r->outputs[0]->path);
  }
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

bool build_rule_plus_dependencies(struct all_targets **all, struct rule *r,
                                  int *num_to_build, int *num_built) {
  if (!r) return false;
  if (r->status == unknown) {
    determine_rule_cleanliness(all, r, num_to_build);
  }
  if (r->status == failed) {
    verbose_printf("already failed once: %s\n", r->command);
    return true;
  }
  if (r->status == dirty) {
    for (int i=0;i<r->num_inputs;i++) {
      if (build_rule_plus_dependencies(all, r->inputs[i]->rule,
                                       num_to_build, num_built)) {
        return true;
      }
    }

    printf("%d/%d: ", *num_built+1, *num_to_build);
    if (!run_rule(all, r)) {
      printf("  Error running \"%s\" (%s:%d)\n",
             r->command, r->bilgefile_path, r->bilgefile_linenum);
      r->status = failed;
      return true;
    }
    *num_built += 1;
    r->status = built;

    char *donefile = done_name(r->bilgefile_path);
    FILE *f = fopen(donefile, "w");
    if (!f) error(1,errno,"oopse");
    fprint_bilgefile(f, *all, r->bilgefile_path);
    fclose(f);
    free(donefile);
  }
  return false;
}

void build_all(struct all_targets **all) {
  bool done = false;
  struct all_targets *tt = *all;
  while (tt) {
    if (tt->t->rule) tt->t->rule->status = unknown;
    tt = tt->next;
  }
  int num_to_build = 0, num_built = 0;
  while (!done) {
    tt = *all;
    while (tt) {
      determine_rule_cleanliness(all, tt->t->rule, &num_to_build);
      tt = tt->next;
    }
    bool got_new_bilgefiles = false;
    tt = *all;
    while (tt) {
      int len = strlen(tt->t->path);
      if (len >= 6 && !strcmp(tt->t->path+len-6, ".bilge")) {
        if (tt->t->status == unknown &&
            (!tt->t->rule || tt->t->rule->status != dirty)) {
          /* This is a clean .bilge file, but we still need to parse it! */
          read_bilge_file(all, tt->t->path);
          got_new_bilgefiles = true;
          tt->t->status = built;
        }
      }
      tt = tt->next;
    }
    if (got_new_bilgefiles) continue;
    tt = *all;
    while (tt) {
      int len = strlen(tt->t->path);
      if (len >= 6 && !strcmp(tt->t->path+len-6, ".bilge")) {
        if (tt->t->rule && tt->t->rule->status == dirty) {
          /* This is a dirty .bilge file, so we need to build it! */
          build_rule_plus_dependencies(all, tt->t->rule,
                                       &num_to_build, &num_built);
          got_new_bilgefiles = true;
          break;
        }
      }
      tt = tt->next;
    }
    if (got_new_bilgefiles) continue;
    tt = *all;
    while (tt) {
      build_rule_plus_dependencies(all, tt->t->rule, &num_to_build, &num_built);
      tt = tt->next;
    }
    if (num_built != num_to_build)
      error(1,0,"Failed %d/%d builds", num_to_build-num_built, num_to_build);
    done = true;
  }
}

static void find_latency(struct rule *r) {
  if (!r) return;
  if (r->status != dirty) return;
  if (r->latency_handled) return;
  r->latency_handled = true;
  clock_t maxchild = 0;
  for (int i=0;i<r->num_outputs;i++) {
    find_latency(r->outputs[i]->rule);
    if (r->outputs[i]->rule->latency_estimate > maxchild)
      maxchild = r->outputs[i]->rule->latency_estimate;
  }
  r->latency_estimate = r->build_time + maxchild;
}

static void find_latencies(struct all_targets *all) {
  while (all) {
    find_latency(all->t->rule);
    all = all->next;
  }
}

struct building *build_rule_or_dependency(struct all_targets **all,
                                          struct rule *r,
                                          int *num_to_build) {
  if (!r) return 0;
  if (r->status == unknown) {
    determine_rule_cleanliness(all, r, num_to_build);
  }
  if (r->status == failed) {
    printf("already failed once: %s\n", r->command);
    return 0;
  }
  if (r->status == dirty) {
    bool ready_to_go = true;
    for (int i=0;i<r->num_inputs;i++) {
      if (r->inputs[i]->rule && r->inputs[i]->rule->status == failed) {
        r->status = failed;
        return 0;
      }
      struct building *b = build_rule_or_dependency(all, r->inputs[i]->rule,
                                                    num_to_build);
      if (b) return b;
      if (r->inputs[i]->rule && !(r->inputs[i]->rule->status == clean || r->inputs[i]->rule->status == built)) {
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

void let_us_build(struct all_targets **all, struct rule *r, int *num_to_build,
                  struct building **bs, int num_threads) {
  for (int i=0;i<num_threads;i++) {
    if (!bs[i]) {
      bs[i] = build_rule_or_dependency(all, r, num_to_build);
      if (bs[i]) {
        if (true) {
          pid_t new_pid = fork();
          if (new_pid == 0) {
            run_parallel_rule(bs[i]);
            exit(0);
          }
          bs[i]->pid = new_pid;
        } else {
          run_parallel_rule(bs[i]);
        }
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
}

void parallel_build_all(struct all_targets **all) {
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
  double clocks_per_second = sysconf(_SC_CLK_TCK);

  clock_t total_cpu_time_spent = 0, total_cpu_time_overhead = 0;
  bool have_read_bilge = false;
  struct rule_list *rules = 0;
  listset *bilgefiles_used = 0;
  do {
    bool newstufftobuild = false;
    for (struct all_targets *tt = *all; tt; tt = tt->next) {
      newstufftobuild |=
        determine_rule_cleanliness(all, tt->t->rule, &num_to_build);
    }
    if (newstufftobuild) {
      find_latencies(*all);
      for (struct all_targets *tt = *all; tt; tt = tt->next) {
        if (tt->t->rule && tt->t->rule->status != clean) {
          delete_rule(&rules, tt->t->rule); // way hokey
          insert_rule_by_latency(&rules, tt->t->rule);
        }
      }
    }
    clock_t total_build_time = 0;
    for (struct rule_list *rr = rules; rr; rr = rr->next) {
      if (rr->r->status == dirty || rr->r->status == building)
        total_build_time += rr->r->build_time;
    }
    if (total_build_time/clocks_per_second/num_jobs > 1.0) {
      double build_seconds = total_build_time/clocks_per_second/num_jobs;
      double build_minutes = 0;
      while (build_seconds > 60) {
        build_minutes++;
        build_seconds -= 60;
      }
      find_elapsed_time();
      double total_seconds = elapsed_seconds + build_seconds;
      double total_minutes = elapsed_minutes + build_minutes;
      if (total_seconds > 60) {
        total_minutes += 1;
        total_seconds -= 60;
      }
      printf("Build time remaining: %.0f:%02.0f / %.0f:%02.0f      \r",
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
          total_cpu_time_overhead += bs[i]->overhead_time;
          bs[i]->rule->status = bs[i]->all_done;
          printf("%d/%d [%.2fs]: %s\n", num_built+1, num_to_build,
                 bs[i]->rule->build_time/clocks_per_second, bs[i]->rule->command);
          off_t stdoutlen = lseek(bs[i]->stdouterrfd, 0, SEEK_END);
          off_t myoffset = 0;
          sendfile(1, bs[i]->stdouterrfd, &myoffset, stdoutlen);
          close(bs[i]->stdouterrfd);

          if (bs[i]->all_done == built) {
            struct rule *r = bs[i]->rule;
            delete_rule(&rules, r);
            insert_to_listset(&bilgefiles_used, r->bilgefile_path);

            /* FIXME We should verify that the inputs specified were actually used */
            for (int ii=0;ii<r->num_inputs;ii++) {
              struct target *t = create_target_with_stat(all, r->inputs[ii]->path);
              if (!t) error(1, errno, "Unable to stat input file %s", r->inputs[ii]->path);
              add_input(r, t);
              delete_from_arrayset(&bs[i]->read, r->inputs[ii]->path);
              delete_from_arrayset(&bs[i]->written, r->inputs[ii]->path);
            }
            for (int ii=0;ii<r->num_outputs;ii++) {
              struct target *t = lookup_target(*all, r->outputs[ii]->path);
              if (t) {
                t->last_modified = 0;
                t->size = 0;
              }
              t = create_target_with_stat(all, r->outputs[ii]->path);
              if (!t) error(1, errno, "Unable to stat output file %s", r->outputs[ii]->path);
              t->rule = r;
              add_output(r, t);
              delete_from_arrayset(&bs[i]->read, r->outputs[ii]->path);
              delete_from_arrayset(&bs[i]->written, r->outputs[ii]->path);
            }
            for (char *path = start_iterating(&bs[i]->read); path; path = iterate(&bs[i]->read)) {
              struct target *t = create_target_with_stat(all, path);
              if (!t) error(1, errno, "Unable to input stat file %s", path);
              add_input(r, t);
            }

            for (char *path = start_iterating(&bs[i]->readdir); path; path = iterate(&bs[i]->readdir)) {
              struct target *t = create_target_with_stat(all, path);
              if (!t) error(1, errno, "Unable to stat directory %s", path);
              add_input(r, t);
            }

            for (char *path = start_iterating(&bs[i]->written); path; path = iterate(&bs[i]->written)) {
              struct target *t = lookup_target(*all, path);
              if (t) {
                t->last_modified = 0;
                t->size = 0;
              }
              t = create_target_with_stat(all, path);
              if (t) {
                t->rule = r;
                add_output(r, t);
              }
            }

            num_built++;
          } else if (bs[i]->all_done == failed) {
            printf("OOPS FAILED!\n");
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

    have_read_bilge = false;
    for (struct all_targets *tt = *all; tt; tt = tt->next) {
      int len = strlen(tt->t->path);
      if (len >= 6 && !strcmp(tt->t->path+len-6, ".bilge")) {
        if (tt->t->status == unknown &&
            (!tt->t->rule || (tt->t->rule->status != dirty &&
                              tt->t->rule->status != building))) {
          /* This is a clean .bilge file, but we still need to parse it! */
          read_bilge_file(all, tt->t->path);
          have_read_bilge = true;
          tt->t->status = built;
        }
      }
    }
    for (struct all_targets *tt = *all; tt; tt = tt->next) {
      int len = strlen(tt->t->path);
      if (len >= 6 && !strcmp(tt->t->path+len-6, ".bilge")) {
        if (tt->t->rule && tt->t->rule->status == dirty) {
          /* This is a dirty .bilge file, so we need to build it! */
          let_us_build(all, tt->t->rule, &num_to_build, bs, num_jobs);
          have_read_bilge = true;
        }
      }
    }
    if (have_read_bilge) continue;

    num_to_go = 0;
    for (struct rule_list *rr = rules; rr; rr = rr->next) {
      if (rr->r->status == dirty || rr->r->status == building) {
        let_us_build(all, rr->r, &num_to_build, bs, num_jobs);
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
        fprint_bilgefile(f, *all, bilgefiles_used->path);
        fclose(f);
        free(donefile);
        bilgefiles_used = bilgefiles_used->next;
      }

      exit(1);
    }
  } while (num_to_go || have_read_bilge);

  while (bilgefiles_used) {
    char *donefile = done_name(bilgefiles_used->path);
    FILE *f = fopen(donefile, "w");
    if (!f) error(1,errno,"oopse");
    fprint_bilgefile(f, *all, bilgefiles_used->path);
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
    if (total_cpu_time_overhead > 0.001*total_cpu_time_spent) {
      printf("Build succeeded! %.0f:%05.2f (%.1f%% wasted)\n",
             elapsed_minutes, elapsed_seconds,
             total_cpu_time_overhead/(double)total_cpu_time_spent);
    } else {
      printf("Build succeeded! %.0f:%05.2f      \n",
             elapsed_minutes, elapsed_seconds);
    }
  }
  free(bs);
  sigaction(SIGINT, &oldact, 0);
}
