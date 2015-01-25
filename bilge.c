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
#include "new-build.h"

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
static int clean_me = 0;
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

  char *root = go_to_bilge_top();

  /* the following loop it to make profiling easier */
  const int num_runs_to_profile = 1;
  for (int repeats=0;repeats<num_runs_to_profile;repeats++) {
    struct all_targets all;
    init_hash_table(&all.r, 1000);
    init_hash_table(&all.t, 10000);
    all.ready_list = all.unready_list = all.clean_list = all.failed_list = all.marked_list = 0;
    create_target(&all, "top.bilge");

    if (false) {
      do {
        for (struct target *t = (struct target *)all.t.first; t; t = (struct target *)t->e.next) {
          if (t->status == unknown) {
            t->status = built;
            int len = strlen(t->path);
            if (len >= 6 && !strcmp(t->path+len-6, ".bilge")) {
              read_bilge_file(&all, t->path);
            }
          }
        }
        build_marked(&all, root);
        mark_bilgefiles(&all);
      } while (all.marked_list);
      mark_all(&all);
      build_marked(&all, root);
      exit(0);
    }

    if (clean_me) {
      parallel_build_all(&all, root, cmd_line_args, true);
      clean_all(&all, root);
    } else {
      parallel_build_all(&all, root, cmd_line_args, false);
    }

    if (create_makefile) {
      FILE *f = fopen(create_makefile, "w");
      if (!f) error(1,errno, "Unable to create makefile: %s", create_makefile);
      fprint_makefile(f, &all, root);
      fclose(f);
    }
    if (create_tupfile) {
      FILE *f = fopen(create_tupfile, "w");
      if (!f) error(1,errno, "Unable to create makefile: %s", create_tupfile);
      fprint_tupfile(f, &all, root);
      fclose(f);
    }
    if (create_script) {
      FILE *f = fopen(create_script, "w");
      if (!f) error(1,errno, "Unable to create script: %s", create_script);
      fprint_script(f, &all, root);
      fclose(f);
    }

    /* profiling shows that as much as a third of our CPU time can be
       spent freeing, so we will only do it if we are repeating the
       computation. */
    if (repeats < num_runs_to_profile-1) free_all_targets(&all);
  }
  return 0;
}
