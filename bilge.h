#ifndef BILGE_H
#define BILGE_H

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/types.h>
#include <time.h>

#include "lib/trie.h"

extern int num_jobs; /* number of jobs to run simultaneously */
extern int verbose; /* true if user requests verbose output */
extern int show_output; /* true if user requests to see command output */

inline void verbose_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  if (verbose) vfprintf(stdout, format, args);
  va_end(args);
}

enum target_status {
  unknown,
  being_determined,
  clean,
  built,
  building,
  failed,
  unready, // means that it is dirty but we cannot yet build it
  dirty
};

struct rule;

/* A struct target describes a single thing that could be built.  Or
   it might describe a source file. */
struct target {
  const char *path;
  enum target_status status;
  time_t last_modified;
  off_t size;

  struct rule *rule;
};

/* A struct rule describes a single build process. */
struct rule {
  const char *command;
  const char *working_directory;
  const char *bilgefile_path;
  int bilgefile_linenum;
  enum target_status status;
  bool is_printed;

  int num_inputs, num_explicit_inputs;
  int input_array_size;
  struct target **inputs;
  time_t *input_times;
  off_t *input_sizes;

  int num_outputs, num_explicit_outputs;
  struct target **outputs;
  time_t *output_times;
  off_t *output_sizes;

  double build_time;
  double latency_estimate;
  bool latency_handled;
};

/* The struct all_targets should be an easily searchable set, or even
   better a map from string to struct target.  Instead, I'm just using
   a simple linked list for now. */
struct all_targets {
  struct trie *tr;
  struct target *t;
  struct all_targets *next;
};

/* The struct rule_list is just for optimizing the build.
   Hypothetically, we could add a rule_list to the all_targets
   equivalent, so the rule_list was automagically built when adding
   targets with rules to all_targets. */
struct rule_list {
  struct rule *r;
  struct rule_list *next;
};

struct target *create_target(struct all_targets **all, const char *path);
void free_all_targets(struct all_targets **all);

struct rule *create_rule(const char *command, const char *working_directory);
struct rule *lookup_rule(struct all_targets *all, const char *command,
                         const char *working_directory);
void add_input(struct rule *t, struct target *inp);
void add_output(struct rule *t, struct target *out);
void add_explicit_input(struct rule *r, struct target *dep);
void add_explicit_output(struct rule *r, struct target *dep);

struct target *lookup_target(struct all_targets *, const char *path);
void insert_target(struct all_targets **all, struct target *t);

void read_bilge_file(struct all_targets **all, const char *path);

void print_bilge_file(struct all_targets *all);
void fprint_bilgefile(FILE *f, struct all_targets *tt, const char *bilgefile_path);

struct rule *run_rule(struct all_targets **all, struct rule *r);
void parallel_build_all(struct all_targets **all, const char *root);

char *done_name(const char *bilgefile);

void insert_rule_by_latency(struct rule_list **list, struct rule *r);
void delete_rule(struct rule_list **list, struct rule *r);
void delete_rule_list(struct rule_list **list);

struct trie *git_ls_files();

#endif
