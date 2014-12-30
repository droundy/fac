#ifndef BILGE_H
#define BILGE_H

#include <stdbool.h>
#include <sys/types.h>

enum target_status {
  unknown,
  clean,
  built,
  dirty
};

struct rule;

/* A struct target describes a single thing that could be built.  Or
   it might describe a source file. */
struct target {
  const char *path;
  enum target_status status;
  time_t last_modified;
  struct rule *rule;
};

/* A struct rule describes a single build process. */
struct rule {
  const char *command;
  const char *working_directory;
  enum target_status status;

  int num_inputs;
  struct target **inputs;

  int num_outputs;
  struct target **outputs;
};

/* The struct all_targets should be an easily searchable set, or even
   better a map from string to struct target.  Instead, I'm just using
   a simple linked list for now. */
struct all_targets {
  struct target *t;
  struct all_targets *next;
};

struct target *create_target(const char *path);

struct rule *create_rule(const char *command, const char *working_directory);
void add_input(struct rule *t, struct target *inp);
void add_output(struct rule *t, struct target *out);

struct target *lookup_target(struct all_targets *, const char *path);
void insert_target(struct all_targets **all, struct target *t);

void read_bilge_file(struct all_targets **all, const char *path);

void print_bilge_file(struct all_targets *all);

#endif
