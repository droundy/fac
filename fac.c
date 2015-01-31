#define _BSD_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <popt.h>

#include "fac.h"
#include "new-build.h"

void usage(poptContext optCon, int exitcode, char *error, char *addl) {
  poptPrintUsage(optCon, stderr, 0);
  if (error) fprintf(stderr, "%s: %s\n", error, addl);
  exit(exitcode);
}

const char *root = 0;

int verbose = 0;
int show_output = 0;
int num_jobs = 0;
static int clean_me = 0;
static int continually_build = 0;
extern inline void verbose_printf(const char *format, ...);

char *create_makefile = 0;
char *create_tupfile = 0;
char *create_script = 0;

int main(int argc, const char **argv) {
  struct poptOption optionsTable[] = {
    { "jobs", 'j', POPT_ARG_INT, &num_jobs, 0,
      "the number of jobs to run simultaneously", "JOBS" },
    { "clean", 'c', POPT_ARG_NONE, &clean_me, 0,
      "clean the build outputs", 0 },
    { "continual", 0, POPT_ARG_NONE, &continually_build, 0,
      "keep rebuilding", 0 },
    { "verbose", 'v', POPT_ARG_NONE, &verbose, 0,
      "give verbose output", 0 },
    { "show-output", 'V', POPT_ARG_NONE, &show_output, 0,
      "show command output", 0 },
    { "makefile", 0, POPT_ARG_STRING, &create_makefile, 0,
      "create a makefile", "Makefile" },
    { "tupfile", 0, POPT_ARG_STRING, &create_tupfile, 0,
      "create a tupfile", "TUPFILE" },
    { "script", 0, POPT_ARG_STRING, &create_script, 0,
      "create a build script", "SCRIPTFILE" },
    POPT_AUTOHELP
    { NULL, 0, 0, NULL, 0 }
  };

  poptContext optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
  poptSetOtherOptionHelp(optCon, "[OPTIONS]* [things to build maybe?]");

  while (poptGetNextOpt(optCon) >= 0);

  listset *cmd_line_args = 0;
  const char *arg;
  char *cwd = getcwd(0,0);
  while ((arg = poptGetArg(optCon))) {
    char *abspath = absolute_path(cwd, arg);
    insert_to_listset(&cmd_line_args, abspath);
  }
  free(cwd);
  poptFreeContext(optCon);

  root = go_to_git_top();

  if (continually_build) {
    build_continual(root);
    exit(0);
  }

  struct all_targets all;
  init_all(&all);

  bool still_reading;
  do {
    still_reading = false;
    for (struct target *t = (struct target *)all.t.first; t; t = (struct target *)t->e.next) {
      if (t->status == unknown &&
          (!t->rule ||
           t->rule->status == clean ||
           t->rule->status == built)) {
        t->status = built;
        if (is_facfile(t->path)) {
          still_reading = true;
          read_fac_file(&all, t->path);
        }
      }
    }
    build_marked(&all);
    for (struct target *t = (struct target *)all.t.first; t; t = (struct target *)t->e.next) {
      if (t->status == unknown &&
          (!t->rule ||
           t->rule->status == clean ||
           t->rule->status == built)) {
        t->status = built;
        if (is_facfile(t->path)) {
          still_reading = true;
          read_fac_file(&all, t->path);
        }
      }
    }
    mark_facfiles(&all);
  } while (all.marked_list || still_reading);
  if (!all.r.first) {
    printf("Please add a .fac file containing rules!\n");
    exit(1);
  }
  if (clean_me) {
    clean_all(&all);
    exit(0);
  }
  if (cmd_line_args) {
    while (cmd_line_args) {
      struct target *t = lookup_target(&all, cmd_line_args->path);
      if (t && t->rule) {
        mark_rule(&all, t->rule);
      } else {
        error(1, 0, "No rule to build %s", cmd_line_args->path);
      }
      cmd_line_args = cmd_line_args->next;
    }
  } else {
    mark_all(&all);
  }

  if (create_makefile) {
    FILE *f = fopen(create_makefile, "w");
    if (!f) error(1,errno, "Unable to create makefile: %s", create_makefile);
    fprint_makefile(f, &all);
    fclose(f);
  }
  if (create_tupfile) {
    FILE *f = fopen(create_tupfile, "w");
    if (!f) error(1,errno, "Unable to create makefile: %s", create_tupfile);
    fprint_tupfile(f, &all);
    fclose(f);
  }
  if (create_script) {
    FILE *f = fopen(create_script, "w");
    if (!f) error(1,errno, "Unable to create script: %s", create_script);
    fprint_script(f, &all);
    fclose(f);
  }

  build_marked(&all);
  summarize_build_results(&all);

  /* enable following line to check for memory leaks */
  if (true) free_all_targets(&all);
  return 0;
}
