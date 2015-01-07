#define _GNU_SOURCE

#include "bilge.h"
#include "lib/bigbrother.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

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
  pthread_t thread;
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

  args[0] = "/bin/sh";
  args[1] = "-c";
  args[2] = b->rule->command;
  args[3] = 0;

  int ret = bigbrother_process_arrayset(b->rule->working_directory,
                                        (char **)args, &b->readdir,
                                        &b->read, &b->written, &b->deleted);
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

void determine_rule_cleanliness(struct all_targets **all, struct rule *r,
                                int *num_to_build) {
  if (!r) return;
  if (r->status != unknown) return;
  for (int i=0;i<r->num_inputs;i++) {
    if (r->inputs[i]->rule) {
      if (r->inputs[i]->rule->status == unknown)
        determine_rule_cleanliness(all, r->inputs[i]->rule, num_to_build);
      if (r->inputs[i]->rule->status == dirty ||
          r->inputs[i]->rule->status == built ||
          r->inputs[i]->rule->status == building) {
        printf("::: %s :::\n", r->command);
        printf(" - dirty because %s needs to be rebuilt.\n",
               r->inputs[i]->path);
        r->status = dirty;
        *num_to_build += 1;
        printf("# dirty = %d\n", *num_to_build);
        return;
      }
    }
    if (r->input_times[i]) {
      if (!create_target_with_stat(all, r->inputs[i]->path) ||
          r->input_times[i] != r->inputs[i]->last_modified ||
          r->input_sizes[i] != r->inputs[i]->size) {
        printf("::: %s :::\n", r->command);
        printf(" - dirty because %s has wrong input time.\n",
               r->inputs[i]->path);
        r->status = dirty;
        *num_to_build += 1;
        printf("# dirty = %d\n", *num_to_build);
        return; /* The file is out of date. */
      }
    } else {
      printf("::: %s :::\n", r->command);
      printf(" - dirty because #%d %s has no input time.\n", i, r->inputs[i]->path);
      r->status = dirty;
      *num_to_build += 1;
      printf("# dirty = %d\n", *num_to_build);
      return; /* The file hasn't been built. */
    }
  }
  for (int i=0;i<r->num_outputs;i++) {
    if (r->output_times[i]) {
      if (!create_target_with_stat(all, r->outputs[i]->path) ||
          r->output_times[i] != r->outputs[i]->last_modified ||
          r->output_sizes[i] != r->outputs[i]->size) {
        printf("::: %s :::\n", r->command);
        printf(" - dirty because %s has wrong output time.\n",
               r->outputs[i]->path);
        printf("   compare times %ld with %ld\n",
               r->outputs[i]->last_modified, r->output_times[i]);
        r->status = dirty;
        *num_to_build += 1;
        printf("# dirty = %d\n", *num_to_build);
        return; /* The file is out of date. */
      }
    } else {
      printf("::: %s :::\n", r->command);
      printf(" - dirty because %s has no output time.\n", r->outputs[i]->path);
      r->status = dirty;
      *num_to_build += 1;
      printf("# dirty = %d\n", *num_to_build);
      return; /* The file hasn't been built. */
    }
  }
  if (r->status == unknown) r->status = clean;
}

bool build_rule_plus_dependencies(struct all_targets **all, struct rule *r,
                                  int *num_to_build, int *num_built) {
  if (!r) return false;
  if (r->status == unknown) {
    determine_rule_cleanliness(all, r, num_to_build);
  }
  if (r->status == failed) {
    printf("already failed once: %s\n", r->command);
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
        if (false) {
          pthread_t th;
          pthread_create(&th, 0, run_parallel_rule, bs[i]);
          pthread_detach(th);
        } else if (true) {
          pid_t pid = fork();
          if (pid == 0) {
            pid_t double_id = fork();
            if (double_id == 0) {
              run_parallel_rule(bs[i]);
              exit(0);
            } else {
              exit(0);
            }
          } else {
            waitpid(pid, 0, 0);
          }
        } else {
          run_parallel_rule(bs[i]);
        }
      }
      return;
    }
  }
}

void parallel_build_all(struct all_targets **all) {
  int num_threads = 2;
  struct building **bs = malloc(num_threads*sizeof(struct building *));
  for (int i=0;i<num_threads;i++) bs[i] = 0;

  int num_to_build = 0, num_built = 0, num_failed = 0, num_to_go = 0;

  do {
    struct all_targets *tt = *all;
    while (tt) {
      determine_rule_cleanliness(all, tt->t->rule, &num_to_build);
      tt = tt->next;
    }

    int threads_available = 0;
    for (int i=0;i<num_threads;i++) {
      if (bs[i]) {
        if (bs[i]->all_done != unknown) {
          bs[i]->rule->status = bs[i]->all_done;
          printf("%d/%d: %s\n", num_built+1, num_to_build, bs[i]->rule->command);

          if (bs[i]->all_done == built) {
            struct rule *r = bs[i]->rule;

            /* FIXME We should verify that the outputs specified were actually produced */
            /* FIXME We should verify that the inputs specified were actually used */
            r->num_inputs = 0; // clear the set of inputs so we only rebuild on actual ones.
            r->num_outputs = 0; // clear the set of outputs so we only rebuild on actual ones.
            for (char *path = start_iterating(&bs[i]->read); path; path = iterate(&bs[i]->read)) {
              struct target *t = create_target_with_stat(all, path);
              if (!t) error(1, errno, "Unable to stat file %s", path);
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
              if (!t) error(1, errno, "Unable to stat file %s", path);
              t->rule = r;
              add_output(r, t);
              fflush(stdout);
            }

            char *donefile = done_name(r->bilgefile_path);
            FILE *f = fopen(donefile, "w");
            if (!f) error(1,errno,"oopse");
            fprint_bilgefile(f, *all, r->bilgefile_path);
            fclose(f);
            free(donefile);
            num_built++;
          } else if (bs[i]->all_done == failed) {
            printf("OOPS FAILED!\n");
            num_failed++;
          } else {
            error(1,0,"what the heck? %d\n", bs[i]->all_done);
          }
          munmap(bs[i], sizeof(struct building));
          bs[i] = 0;
          threads_available++;
        }
      } else {
        threads_available++;
      }
    }

    tt = *all;
    while (tt) {
      int len = strlen(tt->t->path);
      if (len >= 6 && !strcmp(tt->t->path+len-6, ".bilge")) {
        if (tt->t->status == unknown &&
            (!tt->t->rule || (tt->t->rule->status != dirty &&
                              tt->t->rule->status != building))) {
          /* This is a clean .bilge file, but we still need to parse it! */
          read_bilge_file(all, tt->t->path);
          tt->t->status = built;
        }
      }
      tt = tt->next;
    }
    if (threads_available == 0) {
      //waitpid(-1, 0, 0);
      sleep(1); /* FIXME HOKEY and slow */
      continue;
    }
    tt = *all;
    while (tt) {
      int len = strlen(tt->t->path);
      if (len >= 6 && !strcmp(tt->t->path+len-6, ".bilge")) {
        if (tt->t->rule && tt->t->rule->status == dirty) {
          /* This is a dirty .bilge file, so we need to build it! */
          let_us_build(all, tt->t->rule, &num_to_build, bs, num_threads);
        }
      }
      tt = tt->next;
    }

    num_to_go = 0;
    tt = *all;
    while (tt) {
      determine_rule_cleanliness(all, tt->t->rule, &num_to_build);
      if (tt->t->rule) {
        if (tt->t->rule->status == dirty || tt->t->rule->status == building) {
          let_us_build(all, tt->t->rule, &num_to_build, bs, num_threads);
          num_to_go++;
        }
      }
      tt = tt->next;
    }
    printf("I have %d still to build... vs %d\n", num_to_go, num_to_build);
    sleep(1);
    printf("still working...\n");
  } while (num_to_go);
  if (num_failed) {
    printf("Failed %d/%d builds, succeeded %d/%d builds\n", num_failed, num_to_build, num_built, num_to_build);
    exit(1);
  } else {
    printf("Build succeeded!\n");
  }
}
