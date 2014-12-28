#ifndef BILGE_H
#define BILGE_H

#include <stdbool.h>

enum target_status {
  unknown,
  clean,
  built,
  dirty
};

/* A struct target describes a single thing that could be built.  Or
   it might describe a source file. */
struct target {
  const char *path;
  enum target_status status;

  int num_deps;
  struct target **deps;
};

/* The struct all_targets should be an easily searchable set, or even
   better a map from string to struct target.  Instead, I'm just using
   a simple linked list for now. */
struct all_targets {
  struct target *t;
  struct all_targets *next;
};

struct target *create_target(const char *path);
void add_dependency(struct target *t, struct target *dep);

struct target *lookup_target(struct all_targets *, const char *path);
void insert_target(struct all_targets **all, struct target *t);

#endif
