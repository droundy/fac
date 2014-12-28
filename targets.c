#include "bilge.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct target *create_target(const char *path) {
  struct target *t = malloc(sizeof(struct target));
  t->path = path;
  t->status = unknown;
  t->num_deps = 0;
  t->deps = 0;
  return t;
}

void add_dependency(struct target *t, struct target *dep) {
  t->deps = realloc(t->deps, sizeof(struct target *)*(t->num_deps+1));
  t->deps[t->num_deps] = dep;
  t->num_deps += 1;
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

