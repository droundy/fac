#ifndef NEW_BUILD_H
#define NEW_BUILD_H

#include "fac.h"

void mark_facfiles(struct all_targets *all);
void mark_all(struct all_targets *all);
void mark_rule(struct all_targets *all, struct rule *r);

void summarize_build_results(struct all_targets *all);

struct cmd_args {
  const char *create_makefile, *create_tupfile, *create_script, *log_directory;
  bool clean, continual;
  listset *targets_requested;
};

void do_actual_build(struct cmd_args *args);

#endif
