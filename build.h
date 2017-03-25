#ifndef NEW_BUILD_H
#define NEW_BUILD_H

#include "fac.h"

/* Explanation: I use a global variable for dry_run rather than
   putting it into cmd_args (which would be reasonable) because I want
   there to be *NO* risk that we fail to pass dry_run along.  Also, it
   is convenient. */
extern int dry_run; // true if we do not want to do any real building.

void initialize_starting_time();

void mark_facfiles(struct all_targets *all);
void mark_all(struct all_targets *all);

enum strictness {
  normal,
  strict,
  exhaustive
};

struct cmd_args {
  const char *create_dotfile, *create_makefile, *create_tupfile,
    *create_script, *create_tarball, *log_directory;
  const char **include_in_tar;
  bool clean, continual, git_add_files;
  enum strictness strictness;
  listset *targets_requested;
};

void do_actual_build(struct cmd_args *args);

#endif
