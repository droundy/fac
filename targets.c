#include "bilge.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static char *mycopy(const char *str) {
  char *out = malloc(strlen(str)+1);
  strcpy(out, str);
  return out;
}

struct target *create_target(struct all_targets **all, const char *path) {
  struct target *t = lookup_target(*all, path);
  if (!t) {
    t = malloc(sizeof(struct target));
    t->path = mycopy(path);
    t->status = unknown;
    t->rule = 0;
    t->last_modified = 0;
    t->size = 0;
    insert_target(all, t);
  }
  return t;
}

struct rule *create_rule(const char *command, const char *working_directory) {
  struct rule *r = malloc(sizeof(struct rule));
  r->command = mycopy(command);
  r->working_directory = mycopy(working_directory);
  r->status = unknown;
  r->num_inputs = r->num_outputs = 0;
  r->inputs = r->outputs = 0;
  r->input_times = r->output_times = 0;
  r->input_sizes = r->output_sizes = 0;
  r->bilgefile_path = 0;
  r->bilgefile_linenum = 0;
  return r;
}

struct rule *lookup_rule(struct all_targets *all, const char *command,
                         const char *working_directory) {
  while (all) {
    if (all->t->rule &&
        !strcmp(all->t->rule->command, command) &&
        !strcmp(all->t->rule->working_directory, working_directory))
      return all->t->rule;
    all = all->next;
  }
  return 0;
}

void add_input(struct rule *r, struct target *dep) {
  for (int i=0;i<r->num_inputs;i++) {
    if (r->inputs[i] == dep) return;
  }

  r->inputs = realloc(r->inputs, sizeof(struct target *)*(r->num_inputs+1));
  r->input_times = realloc(r->input_times, sizeof(time_t)*(r->num_inputs+1));
  r->input_sizes = realloc(r->input_sizes, sizeof(off_t)*(r->num_inputs+1));
  r->inputs[r->num_inputs] = dep;
  r->input_times[r->num_inputs] = 0;
  r->input_sizes[r->num_inputs] = 0;
  r->num_inputs += 1;
}

void add_output(struct rule *r, struct target *dep) {
  for (int i=0;i<r->num_outputs;i++) {
    if (r->outputs[i] == dep) return;
  }

  r->outputs = realloc(r->outputs, sizeof(struct target *)*(r->num_outputs+1));
  r->output_times = realloc(r->output_times, sizeof(time_t)*(r->num_outputs+1));
  r->output_sizes = realloc(r->output_sizes, sizeof(off_t)*(r->num_outputs+1));
  r->outputs[r->num_outputs] = dep;
  r->output_times[r->num_outputs] = 0;
  r->output_sizes[r->num_outputs] = 0;
  r->num_outputs += 1;
}

struct target *lookup_target(struct all_targets *all, const char *path) {
  while (all) {
    if (!strcmp(all->t->path, path)) return all->t;
    all = all->next;
  }
  return 0;
}

void insert_target(struct all_targets **all, struct target *t) {
  assert(!lookup_target(*all, t->path));
  struct all_targets *n = malloc(sizeof(struct all_targets));
  n->next = *all;
  n->t = t;
  *all = n;
}

