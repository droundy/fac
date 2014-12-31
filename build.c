#include "bilge.h"
#include "lib/bigbrother.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>

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

struct rule *run_rule(struct all_targets **all, struct rule *r) {
  struct rule *out = r; //create_rule(r->command, r->working_directory);
  listset *read_set = 0, *written_set = 0, *deleted_set = 0;
  const char **args = malloc(4*sizeof(char *));
  args[0] = "/bin/sh";
  args[1] = "-c";
  args[2] = r->command;
  args[3] = 0;
  printf("%s\n", r->command);
  int ret = bigbrother_process(r->working_directory,
                               (char **)args,
                               &read_set, &written_set, &deleted_set);
  free(args);
  if (ret != 0) {
    free_listset(read_set);
    free_listset(written_set);
    free_listset(deleted_set);
    return 0;
  }

  listset *s = read_set;
  while (s != NULL) {
    struct target *t = create_target_with_stat(all, s->path);
    if (!t) error(1, errno, "Unable to stat file %s", t->path);
    add_input(out, t);
    s = s->next;
  }

  s = written_set;
  while (s != NULL) {
    struct target *t = lookup_target(*all, s->path);
    if (t) {
      t->last_modified = 0;
      t->size = 0;
    }
    create_target_with_stat(all, s->path);
    if (!t) error(1, errno, "Unable to stat file %s", t->path);
    t->rule = out;
    add_output(out, t);
    s = s->next;
  }

  free_listset(read_set);
  free_listset(written_set);
  free_listset(deleted_set);
  return out;
}

void build_all(struct all_targets **all) {
  struct all_targets *tt = *all;
  int num_to_build = 0;
  while (tt) {
    if (tt->t->rule) tt->t->rule->status = unknown;
    tt = tt->next;
  }
  tt = *all;
  while (tt) {
    if (tt->t->rule && tt->t->rule->status == unknown) {
      struct rule *r = tt->t->rule;
      printf("::: %s :::\n", r->command);
      for (int i=0;i<r->num_inputs;i++) {
        if (r->input_times[i]) {
          if (!create_target_with_stat(all, r->inputs[i]->path) ||
              r->input_times[i] != r->inputs[i]->last_modified ||
              r->input_sizes[i] != r->inputs[i]->size) {
            printf(" - dirty because %s has wrong input time.\n",
                   r->inputs[i]->path);
            r->status = dirty;
            num_to_build++;
            break; /* The file is out of date. */
          }
        } else {
          printf(" - dirty because %s has no input time.", r->inputs[i]->path);
          r->status = dirty;
          num_to_build++;
          break; /* The file hasn't been built. */
        }
      }
      for (int i=0;i<r->num_outputs;i++) {
        if (r->output_times[i]) {
          if (!create_target_with_stat(all, r->outputs[i]->path) ||
              r->output_times[i] != r->outputs[i]->last_modified ||
              r->output_sizes[i] != r->outputs[i]->size) {
            printf(" - dirty because %s has wrong output time.\n",
                   r->outputs[i]->path);
            printf("   compare times %ld with %ld\n",
                   r->outputs[i]->last_modified, r->output_times[i]);
            r->status = dirty;
            num_to_build++;
            break; /* The file is out of date. */
          }
        } else {
          printf(" - dirty because %s has no output time.\n", r->outputs[i]->path);
          r->status = dirty;
          num_to_build++;
          break; /* The file hasn't been built. */
        }
      }
      if (r->status == unknown) r->status = clean;
    }
    tt = tt->next;
  }
  tt = *all;
  while (tt) {
    if (tt->t->rule) {
      switch (tt->t->rule->status) {
      case unknown:
      case dirty:
        {
          struct rule *r = run_rule(all, tt->t->rule);
          if (!r) error_at_line(1, 0,
                                tt->t->rule->bilgefile_path,
                                tt->t->rule->bilgefile_linenum,
                                "Error running \"%s\"", tt->t->rule->command);
          /*
            printf("| %s\n", r->command);
            for (int i=0; i<r->num_inputs; i++) {
            printf("< %s\n", r->inputs[i]->path);
            }
            for (int i=0; i<r->num_outputs; i++) {
            printf("> %s\n", r->outputs[i]->path);
            }
            printf("\n");
          */
          r->status = built;

          char *donefile = done_name(r->bilgefile_path);
          FILE *f = fopen(donefile, "w");
          if (!f) error(1,errno,"oopse");
          fprint_bilgefile(f, *all, r->bilgefile_path);
          fclose(f);
          free(donefile);
        }
        break;
      case clean:
      case built:
        printf("already built: %s\n", tt->t->rule->command);
        /* nothing to do */
      }
    }
    tt = tt->next;
  }
}
