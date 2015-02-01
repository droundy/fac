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

const char *root = 0;

int verbose = 0;
int show_output = 0;
int num_jobs = 0;

static int clean_me = 0;
static int continually_build = 0;

extern inline void verbose_printf(const char *format, ...);

static char *create_makefile = 0;
static char *create_tupfile = 0;
static char *create_script = 0;

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
  poptSetOtherOptionHelp(optCon, "[OPTIONS] [TARGETS]");

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

  struct cmd_args args;
  args.create_makefile = create_makefile;
  args.create_tupfile = create_tupfile;
  args.create_script = create_script;
  args.clean = clean_me;
  args.continual = continually_build;
  args.targets_requested = cmd_line_args;

  // The following line does whatever is requested of us.
  do_actual_build(&args);
  return 0;
}
