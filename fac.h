#ifndef LOON_H
#define LOON_H

#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>

#include "lib/sha1.h"
#include "lib/listset.h"
#include "lib/iterablehash.h"

extern int num_jobs; /* number of jobs to run simultaneously */
extern int show_output; /* true if user requests to see command output */

extern const char *root;
static inline bool is_in_root(const char *path) {
  int lenpath = strlen(path);
  int lenroot = strlen(root);
  if (lenpath < lenroot + 1) return false;
  return path[lenroot] == '/' && !memcmp(path, root, lenroot);
}
static inline bool is_in_gitdir(const char *path) {
  int lenpath = strlen(path);
  int lenroot = strlen(root);
  if (lenpath < lenroot + 6) return false;
  return path[lenroot] == '/' &&
       path[lenroot+1] == '.' &&
       path[lenroot+2] == 'g' &&
       path[lenroot+3] == 'i' &&
       path[lenroot+4] == 't' &&
       path[lenroot+5] == '/' && !memcmp(path, root, lenroot);
}
static inline bool is_facfile(const char *path) {
  int len = strlen(path);
  return len >= 4 && !strcmp(path+len-4, ".fac");
}

enum target_status {
  unknown = 0,
  being_determined,
  clean,
  built,
  building,
  failed,
  marked, // means unknown, but want to build it
  unready, // means that it is dirty but we cannot yet build it
  dirty,
  num_statuses // this is not an actual status, but counts the number of values
};

static inline const char *pretty_status(enum target_status status) {
  switch (status) {
  case unknown: return "unknown";
  case unready: return "unready";
  case dirty: return "dirty";
  case marked: return "marked";
  case failed: return "failed";
  case building: return "building";
  case clean: return "clean";
  case built: return "built";
  case being_determined: return "being_determined";
  case num_statuses: return "num_statuses";
  }
  return "ERROR-ERROR";
}

struct rule;

struct hashstat {
  time_t time;
  off_t size;
  sha1hash hash;
};

/* A struct target describes a single thing that could be built.  Or
   it might describe a source file. */
struct target {
  struct hash_entry e;
  enum target_status status;
  struct hashstat stat;

  bool is_file, is_dir, is_symlink;
  bool is_in_git, is_printed;

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
  struct hashstat *input_stats;
  sha1hash env;

  int num_outputs, num_explicit_outputs;
  struct target **outputs;
  struct hashstat *output_stats;

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
  struct rule *lists[num_statuses];
  int num_with_status[num_statuses];
  double estimated_times[num_statuses];
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

void set_status(struct all_targets *all, struct rule *r, enum target_status status);

struct target *lookup_target(struct all_targets *, const char *path);

void read_fac_file(struct all_targets *all, const char *path);

char *absolute_path(const char *dir, const char *rel);

void print_fac_file(struct all_targets *all);
void fprint_facfile(FILE *f, struct all_targets *tt, const char *facfile_path);

void fprint_dot(FILE *f, struct all_targets *tt);
void fprint_makefile(FILE *f, struct all_targets *tt);
void fprint_tupfile(FILE *f, struct all_targets *tt);
void fprint_script(FILE *f, struct all_targets *tt);

struct rule *run_rule(struct all_targets *all, struct rule *r);
void parallel_build_all(struct all_targets *all,
                        listset *cmd_line_args, bool facfiles_only);
void clean_all(struct all_targets *all);

char *done_name(const char *facfile);

char *go_to_git_top();
void git_add(const char *path);
void add_git_files(struct all_targets *all);

void init_all(struct all_targets *all);

static inline const char *pretty_path(const char *path) {
  int len = strlen(root);
  if (path[len] == '/' && !memcmp(path, root, len)) {
    return path + len + 1;
  }
  return path;
}
static inline const char *pretty_rule(struct rule *r) {
  if (r->num_outputs && strlen(pretty_path(r->outputs[0]->path)) <= strlen(r->command)) {
    return pretty_path(r->outputs[0]->path);
  }
  return r->command;
}

/* pretty_reason is a way of describing a rule in terms of why it
   needs to be built.  If the rule is always built by default, it
   gives the same output that pretty_rule does.  However, if it is a
   non-default rule, it selects an output which is actually needed to
   describe why it needs to be built. */
static inline const char *pretty_reason(struct rule *r) {
  if (r->is_default) return pretty_rule(r);
  for (int i=0;i<r->num_outputs;i++) {
    for (int j=0;j<r->outputs[i]->num_children;j++) {
      if (r->outputs[i]->children[j]->status == unready ||
          r->outputs[i]->children[j]->status == failed) {
        // we need to build outputs[i] so it is a reason we need this!
        return pretty_path(r->outputs[i]->path);
      }
    }
  }
  return pretty_rule(r); // ?!
}

#endif
