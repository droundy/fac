#define _XOPEN_SOURCE 700

#include "fac.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

void insert_target(struct all_targets *all, struct target *t);

struct target *create_target(struct all_targets *all, const char *path) {
  struct target *t = lookup_target(all, path);
  if (!t) {
    t = malloc(sizeof(struct target) + strlen(path)+1);
    strcpy((char *)t->path, path);
    t->e.key = t->path;
    t->is_in_git = false;
    t->is_file = false;
    t->is_dir = false;
    t->is_symlink = false;
    t->e.next = 0;
    t->status = unknown;
    t->rule = 0;
    t->stat.time = 0;
    t->stat.size = 0;
    t->stat.hash.abc.a = t->stat.hash.abc.b = t->stat.hash.abc.c = 0;
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

struct rule *create_rule(struct all_targets *all, const char *facfile_path,
                         const char *command, const char *working_directory) {
  struct rule *r = malloc(sizeof(struct rule) + strlen(facfile_path) +
                          strlen(command) + strlen(working_directory) + 3);
  strcpy((char *)r->command, command);
  r->working_directory = r->command + strlen(command)+1;
  strcpy((char *)r->working_directory, working_directory);
  r->facfile_path = r->command + strlen(command) + strlen(working_directory)+2;
  strcpy((char *)r->facfile_path, facfile_path);

  r->e.key = rule_key(command, working_directory);
  r->e.next = 0;
  r->status_next = all->lists[unknown];
  r->status_prev = &all->lists[unknown];
  r->status = unknown;
  all->lists[unknown] = r;
  if (r->status_next) r->status_next->status_prev = &r->status_next;
  all->num_with_status[unknown]++;

  r->env.abc.a = r->env.abc.b = r->env.abc.c = 0;

  r->num_inputs = r->num_outputs = 0;
  r->num_explicit_inputs = r->num_explicit_outputs = 0;
  r->input_array_size = 0;
  r->inputs = r->outputs = 0;
  r->input_stats = r->output_stats = 0;
  r->facfile_linenum = 0;
  /* Initial guess of a second for build_time helps us build commands
     with dependencies first, even if we don't know how long those
     will actually take. */
  r->build_time = 1;
  r->latency_estimate = 0;
  r->latency_handled = false;
  r->is_default = true;

  r->num_cache_suffixes = r->num_cache_prefixes = 0;
  r->cache_suffixes_reversed = r->cache_prefixes = 0;

  add_to_hash(&all->r, &r->e);

  add_cache_prefix(r, "/tmp/prof");
  add_cache_prefix(r, "/sys/");
  add_cache_prefix(r, "/dev/");
  add_cache_prefix(r, "/proc/");
  return r;
}

void set_status(struct all_targets *all, struct rule *r, enum target_status status) {
  if (r->status == status) return;

  /* remove from its former list */
  if (r->status_prev) {
    (*r->status_prev) = r->status_next;
    all->num_with_status[r->status]--;
    all->estimated_times[r->status] -= r->build_time;
  } else if (status) {
    fprintf(stderr, "%s was in no list at all but had status %s!",
            pretty_rule(r), pretty_status(r->status));
    exit(1);
  }
  if (r->status_next) {
    r->status_next->status_prev = r->status_prev;
  }
  r->status = status;
  r->status_next = all->lists[status];
  r->status_prev = &all->lists[status];
  if (r->status_next) r->status_next->status_prev = &r->status_next;
  all->lists[status] = r;
  all->num_with_status[status]++;
  all->estimated_times[status] += r->build_time;
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
  for (int i=0;i<len;i++) reversed[i] = suffix[len-i-1];
  r->cache_suffixes_reversed[r->num_cache_suffixes] = reversed;
  r->num_cache_suffixes++;
}

void add_input(struct rule *r, struct target *dep) {
  for (int i=0;i<r->num_inputs;i++) {
    if (r->inputs[i] == dep) {
      r->input_stats[i] = dep->stat;
      return;
    }
  }

  if (r->input_array_size == r->num_inputs) {
    if (r->input_array_size) {
      r->input_array_size *= 2;
    } else {
      r->input_array_size = 16;
    }
    r->inputs = realloc(r->inputs, sizeof(struct target *)*(r->input_array_size));
    r->input_stats = realloc(r->input_stats, sizeof(struct hashstat)*(r->input_array_size));
  }

  r->inputs[r->num_inputs] = dep;
  r->input_stats[r->num_inputs] = dep->stat;
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
    if (r->outputs[i] == dep) {
      r->output_stats[i] = dep->stat;
      return;
    }
  }

  r->outputs = realloc(r->outputs, sizeof(struct target *)*(r->num_outputs+1));
  r->output_stats = realloc(r->output_stats, sizeof(struct hashstat)*(r->num_outputs+1));
  r->outputs[r->num_outputs] = dep;
  r->output_stats[r->num_outputs] = dep->stat;
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
    free_hash_table(&all->t);
  }
  {
    struct rule *next;
    for (struct rule *r = (struct rule *)all->r.first; r; r = next) {
      free((void *)r->e.key);
      for (int i=0;i<r->num_cache_suffixes;i++)
        free((void *)r->cache_suffixes_reversed[i]);
      free(r->cache_suffixes_reversed);
      for (int i=0;i<r->num_cache_prefixes;i++)
        free((void *)r->cache_prefixes[i]);
      free(r->cache_prefixes);
      free(r->inputs);
      free(r->outputs);
      free(r->input_stats);
      free(r->output_stats);
      next = (struct rule *)r->e.next;
      free(r);
    }
    free_hash_table(&all->r);
  }
}

struct target *lookup_target(struct all_targets *all, const char *path) {
  return (struct target *)lookup_in_hash(&all->t, path);
}

void insert_target(struct all_targets *all, struct target *t) {
  add_to_hash(&all->t, &t->e);
}

void init_all(struct all_targets *all) {
  init_hash_table(&all->r, 1000);
  init_hash_table(&all->t, 10000);
  for (enum target_status i=0;i<num_statuses;i++) {
    all->lists[i] = 0;
    all->num_with_status[i] = 0;
    all->estimated_times[i] = 0;
  }
  add_git_files(all);
}
