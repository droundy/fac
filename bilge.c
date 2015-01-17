#define _BSD_SOURCE

#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <popt.h>

#include "bilge.h"

char *go_to_bilge_top() {
  while (1) {
    DIR *dir = opendir(".");
    char *dirname = getcwd(0,0);
    struct dirent entry, *result;

    if (strcmp(dirname, "/") == 0)
      error(1, errno, "could not locate top.bilge!");

    if (!dir) error(1, errno, "unable to opendir %s", dirname);

    if (readdir_r(dir, &entry, &result))
      error(1, errno, "error reading from %s", dirname);
    while (result) {
      if (!strcmp("top.bilge", entry.d_name)) {
        closedir(dir);
        return dirname;
      }

      if (readdir_r(dir, &entry, &result))
        error(1, errno, "error reading from %s", dirname);
    }
    closedir(dir);

    if (chdir("..")) error(1, errno, "unable to chdir(..) from %s", dirname);
    free(dirname);
  }
}

void usage(poptContext optCon, int exitcode, char *error, char *addl) {
  poptPrintUsage(optCon, stderr, 0);
  if (error) fprintf(stderr, "%s: %s\n", error, addl);
  exit(exitcode);
}

int verbose = 0;
int show_output = 0;
extern inline void verbose_printf(const char *format, ...);

int main(int argc, const char **argv) {
  struct poptOption optionsTable[] = {
    { "jobs", 'j', POPT_ARG_INT, &num_jobs, 0,
      "the number of jobs to run simultaneously", "JOBS" },
    { "verbose", 'v', POPT_ARG_NONE, &verbose, 0,
      "give verbose output", 0 },
    { "show-output", 'V', POPT_ARG_NONE, &show_output, 0,
      "show command output", 0 },
    POPT_AUTOHELP
    { NULL, 0, 0, NULL, 0 }
  };

  poptContext optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
  poptSetOtherOptionHelp(optCon, "[OPTIONS]* [things to build maybe?]");

  while (poptGetNextOpt(optCon) >= 0);
  poptFreeContext(optCon);

  char *root = go_to_bilge_top();

  /* the following loop it to make profiling easier */
  const int num_runs_to_profile = 1;
  for (int repeats=0;repeats<num_runs_to_profile;repeats++) {
    struct all_targets *all = 0;
    create_target(&all, "top.bilge");
    parallel_build_all(&all, root);
    /* profiling shows that as much as a third of our CPU time can be
       spent freeing, so we will only do it if we are repeating the
       computation. */
    if (repeats < num_runs_to_profile-1) free_all_targets(&all); }

  return 0;
}
