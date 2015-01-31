#ifndef LOON_H
#define LOON_H

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>

#include "lib/trie.h"
#include "lib/listset.h"
#include "lib/iterablehash.h"

extern int num_jobs; /* number of jobs to run simultaneously */
extern int verbose; /* true if user requests verbose output */
extern int show_output; /* true if user requests to see command output */

extern const char *root;
static inline bool is_in_root(const char *path) {
  int lenpath = strlen(path);
  int lenroot = strlen(root);
  if (lenpath < lenroot + 1) return false;
  return path[lenroot] == '/' && !memcmp(path, root, lenroot);
}
static inline bool is_facfile(const char *path) {
  int len = strlen(path);
  return len >= 4 && !strcmp(path+len-4, ".fac");
}

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
  marked, // means unknown, but want to build it
  unready, // means that it is dirty but we cannot yet build it
  dirty
};

struct rule;

/* A struct target describes a single thing that could be built.  Or
   it might describe a source file. */
struct target {
  struct hash_entry e;
  enum target_status status;
  time_t last_modified;
  off_t size;

  bool is_in_git;

  struct rule *rule;
  int num_children, children_size;
  struct rule **children;
  const char path[];
};

/* A struct rule describes a single build process. */
struct rule {
  struct hash_entry e;
  /* The status_next and status_prev fields are used for a
     doubly-linked list of dirty files (or unready files).  Each rule
     can have only one status, and can be in just one status list. */
  struct rule *status_next, **status_prev;
  int facfile_linenum;
  enum target_status status;

  int num_cache_prefixes, num_cache_suffixes;
  const char **cache_suffixes_reversed;
  const char **cache_prefixes;

  int num_inputs, num_explicit_inputs;
  int input_array_size;
  struct target **inputs;
  time_t *input_times;
  off_t *input_sizes;

  int num_outputs, num_explicit_outputs;
  struct target **outputs;
  time_t *output_times;
  off_t *output_sizes;

  double build_time, old_build_time;
  double latency_estimate;
  bool latency_handled, is_printed, is_default;

  const char *working_directory;
  const char *facfile_path;
  const char command[];
};

/* The struct all_targets should be an easily searchable set, or even
   better a map from string to struct target.  Instead, I'm just using
   a simple linked list for now. */
struct all_targets {
  struct hash_table t, r;
  struct rule *ready_list, *unready_list, *clean_list, *failed_list, *marked_list, *running_list;
  int ready_num, unready_num, failed_num, built_num;
  double estimated_time;
};

/* The struct rule_list is just for optimizing the build.
   Hypothetically, we could add a rule_list to the all_targets
   equivalent, so the rule_list was automagically built when adding
   targets with rules to all_targets. */
struct rule_list {
  struct rule *r;
  struct rule_list *next;
};

struct target *create_target(struct all_targets *all, const char *path);
void free_all_targets(struct all_targets *all);

struct rule *create_rule(struct all_targets *all, const char *facfile_path,
                         const char *command, const char *working_directory);
struct rule *lookup_rule(struct all_targets *all, const char *command,
                         const char *working_directory);
void add_input(struct rule *t, struct target *inp);
void add_output(struct rule *t, struct target *out);
void add_explicit_input(struct rule *r, struct target *dep);
void add_explicit_output(struct rule *r, struct target *dep);
void add_cache_prefix(struct rule *r, const char *prefix);
void add_cache_suffix(struct rule *r, const char *suffix);
bool is_interesting_path(struct rule *r, const char *path);

void put_rule_into_status_list(struct rule **list, struct rule *r);

struct target *lookup_target(struct all_targets *, const char *path);

void read_fac_file(struct all_targets *all, const char *path);

char *absolute_path(const char *dir, const char *rel);

void print_fac_file(struct all_targets *all);
void fprint_facfile(FILE *f, struct all_targets *tt, const char *facfile_path);

void fprint_makefile(FILE *f, struct all_targets *tt);
void fprint_tupfile(FILE *f, struct all_targets *tt);
void fprint_script(FILE *f, struct all_targets *tt);

struct rule *run_rule(struct all_targets *all, struct rule *r);
void parallel_build_all(struct all_targets *all,
                        listset *cmd_line_args, bool facfiles_only);
void clean_all(struct all_targets *all);

char *done_name(const char *facfile);

void insert_rule_by_latency(struct rule_list **list, struct rule *r);
void delete_rule(struct rule_list **list, struct rule *r);
void delete_rule_list(struct rule_list **list);

char *go_to_git_top();
void add_git_files(struct all_targets *all);

void init_all(struct all_targets *all);

#endif
