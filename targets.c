#define _XOPEN_SOURCE 700

#include "bilge.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

void insert_target(struct all_targets *all, struct target *t);

struct target *create_target(struct all_targets *all, const char *path) {
  struct target *t = lookup_target(all, path);
  if (!t) {
    t = malloc(sizeof(struct target));
    t->path = strdup(path);
    t->e.key = t->path;
    t->e.next = 0;
    t->status = unknown;
    t->rule = 0;
    t->last_modified = 0;
    t->size = 0;
    t->num_children = t->children_size = 0;
    t->children = 0;
    insert_target(all, t);
  }
  return t;
}

static char *rule_key(const char *command, const char *working_directory) {
  int lencommand = strlen(command);
  char *key = malloc(lencommand+strlen(working_directory)+2); /* 1 to spare */
  strcpy(key, command);
  key[lencommand] = 1; /* separate command from working directory with a 1 byte */
  key[lencommand+1] = 0;
  strcat(key, working_directory);
  return key;
}

struct rule *create_rule(struct all_targets *all,
                         const char *command, const char *working_directory) {
  struct rule *r = malloc(sizeof(struct rule));
  r->command = strdup(command);
  r->working_directory = strdup(working_directory);

  r->e.key = rule_key(command, working_directory);
  r->e.next = 0;
  r->status_next = 0;
  r->status_prev = 0;

  r->status = unknown;
  r->num_inputs = r->num_outputs = 0;
  r->num_explicit_inputs = r->num_explicit_outputs = 0;
  r->input_array_size = 0;
  r->inputs = r->outputs = 0;
  r->input_times = r->output_times = 0;
  r->input_sizes = r->output_sizes = 0;
  r->bilgefile_path = 0;
  r->bilgefile_linenum = 0;
  /* Initial guess of a second for build_time helps us build commands
     with dependencies first, even if we don't know how long those
     will actually take. */
  r->build_time = 1;
  r->latency_estimate = 0;
  r->latency_handled = false;

  r->num_cache_suffixes = r->num_cache_prefixes = 0;
  r->cache_suffixes_reversed = r->cache_prefixes = 0;

  add_to_hash(&all->r, &r->e);
  return r;
}

void put_rule_into_status_list(struct rule **list, struct rule *r) {
  /* remove from its former list */
  if (r->status_prev) {
    (*r->status_prev) = r->status_next;
  }
  if (r->status_next) {
    r->status_next->status_prev = r->status_prev;
  }
  r->status_next = *list;
  r->status_prev = list;
  if (r->status_next) r->status_next->status_prev = &r->status_next;
  *list = r;
}

struct rule *lookup_rule(struct all_targets *all, const char *command,
                         const char *working_directory) {
  char *key = rule_key(command, working_directory);
  struct hash_entry *v = lookup_in_hash(&all->r, key);
  free(key);
  return (struct rule *)v;
}

void add_cache_prefix(struct rule *r, const char *prefix) {
  r->cache_prefixes = realloc(r->cache_prefixes, sizeof(char *)*(r->num_cache_prefixes+1));
  r->cache_prefixes[r->num_cache_prefixes] = strdup(prefix);
  r->num_cache_prefixes++;
}

void add_cache_suffix(struct rule *r, const char *suffix) {
  r->cache_suffixes_reversed = realloc(r->cache_suffixes_reversed,
                                       sizeof(char *)*(r->num_cache_suffixes+1));
  const int len = strlen(suffix);
  char *reversed = malloc(len+1);
  reversed[len] = 0;
  for (int i=0;i<len;i++) reversed[i] = suffix[len-i];
  r->cache_suffixes_reversed[r->num_cache_suffixes] = reversed;
  r->num_cache_suffixes++;
}

void add_input(struct rule *r, struct target *dep) {
  for (int i=0;i<r->num_inputs;i++) {
    if (r->inputs[i] == dep) return;
  }

  if (r->input_array_size == r->num_inputs) {
    if (r->input_array_size) {
      r->input_array_size *= 2;
    } else {
      r->input_array_size = 16;
    }
    r->inputs = realloc(r->inputs, sizeof(struct target *)*(r->input_array_size));
    r->input_times = realloc(r->input_times, sizeof(time_t)*(r->input_array_size));
    r->input_sizes = realloc(r->input_sizes, sizeof(off_t)*(r->input_array_size));
  }

  r->inputs[r->num_inputs] = dep;
  r->input_times[r->num_inputs] = 0;
  r->input_sizes[r->num_inputs] = 0;
  r->num_inputs += 1;

  dep->num_children++;
  if (dep->num_children > dep->children_size) {
    dep->children_size = dep->children_size*10+1;
    dep->children = realloc(dep->children, dep->children_size * sizeof(struct rule *));
  }
  dep->children[dep->num_children-1] = r;
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

void add_explicit_output(struct rule *r, struct target *dep) {
  add_output(r, dep);
  r->num_explicit_outputs = r->num_outputs;
}

void add_explicit_input(struct rule *r, struct target *dep) {
  add_input(r, dep);
  r->num_explicit_inputs = r->num_inputs;
}

void free_all_targets(struct all_targets *all) {
  {
    struct target *next;
    for (struct target *t = (struct target *)all->t.first; t; t = next) {
      free(t->children);
      next = (struct target *)t->e.next;
      free(t);
    }
  }
  {
    struct rule *next;
    for (struct rule *r = (struct rule *)all->r.first; r; r = next) {
      free(r->cache_suffixes_reversed);
      free(r->cache_prefixes);
      free(r->inputs);
      free(r->outputs);
      free(r->input_times);
      free(r->output_times);
      free(r->input_sizes);
      free(r->output_sizes);
      next = (struct rule *)r->e.next;
      free(r);
    }
  }
}

struct target *lookup_target(struct all_targets *all, const char *path) {
  return (struct target *)lookup_in_hash(&all->t, path);
}

void insert_target(struct all_targets *all, struct target *t) {
  add_to_hash(&all->t, &t->e);
}

void insert_rule_by_latency(struct rule_list **list, struct rule *r) {
  while (*list && (*list)->r->latency_estimate > r->latency_estimate) {
    list = &(*list)->next;
  }
  struct rule_list *n = malloc(sizeof(struct rule_list));
  n->next = *list;
  n->r = r;
  *list = n;
}

void delete_rule(struct rule_list **list, struct rule *r) {
  while (*list) {
    if ((*list)->r == r) {
      struct rule_list *to_be_deleted = *list;
      *list = (*list)->next;
      free(to_be_deleted);
      return;
    }
    list = &((*list)->next);
  }
}

void delete_rule_list(struct rule_list **list) {
  struct rule_list *l = *list;
  *list = 0;
  while (l) {
    struct rule_list *to_be_deleted = l;
    l = l->next;
    free(to_be_deleted);
  }
}
