/* Fac build system
   Copyright (C) 2015 David Roundy

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <popt.h>
#include <sys/time.h>

#include "fac.h"
#include "build.h"
#include "version-identifier.h"
#include "errors.h"

const char *root = 0;

int verbose = 0;
int show_output = 0;
int num_jobs = 0;

static int clean_me = 0;
static int show_version = 0;
static int continually_build = 0;
static int git_add_flag = 0;

extern inline void verbose_printf(const char *format, ...);

static char *create_dotfile = 0;
static char *create_makefile = 0;
static char *create_tupfile = 0;
static char *create_script = 0;
static char *create_tarball = 0;
static char *log_directory = 0;

static const char **include_in_tar = 0;

int main(int argc, const char **argv) {
  initialize_starting_time();
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
    { "log-output", 'l', POPT_ARG_STRING, &log_directory, 0,
      "log command output to directory", "LOG_DIRECTORY" },
    { "git-add", 0, POPT_ARG_NONE, &git_add_flag, 0,
      "git add needed files", 0 },
    { "dotfile", 0, POPT_ARG_STRING, &create_dotfile, 0,
      "create a dotfile to visualize dependencies", "Dotfile" },
    { "makefile", 0, POPT_ARG_STRING, &create_makefile, 0,
      "create a makefile", "Makefile" },
    { "tupfile", 0, POPT_ARG_STRING, &create_tupfile, 0,
      "create a tupfile", "TUPFILE" },
    { "script", 0, POPT_ARG_STRING, &create_script, 0,
      "create a build script", "SCRIPTFILE" },
    { "tar", 0, POPT_ARG_STRING, &create_tarball, 0,
      "create a tar archive", "TARNAME.tar" },
    { "include-in-tar", 'i', POPT_ARG_ARGV, &include_in_tar, 0,
      "include file in tarball", "FILENAME" },
    { "version", 'V', POPT_ARG_NONE, &show_version, 0,
      "display the version", 0 },
    POPT_AUTOHELP
    { NULL, 0, 0, NULL, 0 }
  };

  poptContext optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
  poptSetOtherOptionHelp(optCon, "[OPTIONS] [TARGETS]");

  while (poptGetNextOpt(optCon) >= 0);

  if (show_version) {
    printf("fac version %s\n", version_identifier);
    exit(0);
  }

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
  args.include_in_tar = include_in_tar;
  args.create_dotfile = create_dotfile;
  args.create_makefile = create_makefile;
  args.create_tupfile = create_tupfile;
  args.create_script = create_script;
  args.create_tarball = create_tarball;
  args.clean = clean_me;
  args.continual = continually_build;
  args.git_add_files = git_add_flag;
  args.targets_requested = cmd_line_args;
  if (log_directory) log_directory = absolute_path(root, log_directory);
  args.log_directory = log_directory;

  // The following line does whatever is requested of us.
  do_actual_build(&args);
  return 0;
}
