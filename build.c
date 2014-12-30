#include "bilge.h"
#include "lib/bigbrother.h"

#include <stdlib.h>
#include <stdio.h>
#include <error.h>

struct rule *run_rule(struct all_targets **all, struct rule *r) {
  struct rule *out = create_rule(r->command, r->working_directory);
  listset *read_set = 0, *written_set = 0, *deleted_set = 0;
  const char **args = malloc(4*sizeof(char *));
  args[0] = "/bin/sh";
  args[1] = "-c";
  args[2] = r->command;
  args[3] = 0;
  int ret = bigbrother_process(r->working_directory,
                               (char **)args,
                               &read_set, &written_set, &deleted_set);
  if (ret != 0) return 0;

  listset *s = read_set;
  while (s != NULL) {
    add_input(out, create_target(all, s->path));
    listset *d = s;
    s = s->next;
    free(d->path);
    free(d);
  }
  read_set = NULL;

  s = written_set;
  while (s != NULL) {
    struct target *t = create_target(all, s->path);
    t->rule = out;
    add_output(out, t);
    listset *d = s;
    s = s->next;
    free(d->path);
    free(d);
  }

  s = deleted_set;
  while (s != NULL) {
    listset *d = s;
    s = s->next;
    free(d->path);
    free(d);
  }

  return out;
}

void build_all(struct all_targets **all) {
  struct all_targets *t = *all;
  while (t) {
    if (t->t->rule) t->t->rule->status = unknown;
    t = t->next;
  }
  struct all_targets *tt = *all;
  while (tt) {
    if (tt->t->rule && tt->t->rule->status == unknown) {
      struct rule *r = run_rule(all, tt->t->rule);
      if (!r) error_at_line(1, 0,
                            tt->t->rule->bilgefile_path,
                            tt->t->rule->bilgefile_linenum,
                            "Error running \"%s\"", tt->t->rule->command);

      printf("| %s\n", r->command);
      for (int i=0; i<r->num_inputs; i++) {
        printf("< %s\n", r->inputs[i]->path);
      }
      for (int i=0; i<r->num_outputs; i++) {
        printf("> %s\n", r->outputs[i]->path);
      }
      printf("\n");
      tt->t->rule->status = built;
    }
    tt = tt->next;
  }
}
