/* Fac build system
   Copyright (C) 2015,2017 David Roundy

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
#include <sys/time.h>

#include "arguments.h"

#include "fac.h"
#include "build.h"
#include "version-identifier.h"
#include "errors.h"

const char *root = NULL;

int verbose = 0;
int dry_run = 0;
int show_output = 0;
int num_jobs = 0;

static int clean_me = 0;
static int show_version = 0;
static int continually_build = 0;
static int git_add_flag = 0;

extern inline void verbose_printf(const char *format, ...);

static char *create_dotfile = NULL;
static char *create_makefile = NULL;
static char *create_tupfile = NULL;
static char *create_script = NULL;
static char *create_tarball = NULL;
static char *log_directory = NULL;
static char *parse_only = NULL;

static const char **include_in_tar = NULL;

int run_fac(int argc, const char **argv) {
  int am_strict = 0;
  int am_exhaustive = 0;
  int blind = 0;

  int_argument("jobs", 'j', &num_jobs,
               "the number of jobs to run simultaneously", "JOBS");
  no_argument("clean", 'c', (bool *)&clean_me,
              "clean the build outputs");
  no_argument("continual", 0, (bool *)&continually_build,
              "keep rebuilding");
  no_argument("verbose", 'v', (bool *)&verbose,
              "show verbose output");
  string_argument("log-output", 'l', &log_directory,
                  "log command output to directory", "LOG_DIRECTORY");
  no_argument("show-output", 'V', (bool *)&show_output,
              "show command output");
  no_argument("git-add", 0, (bool *)&git_add_flag,
              "git add needed files");
  no_argument("strict", 0, (bool *)&am_strict,
              "require strict dependencies, so first build will succeed");
  no_argument("exhaustive", 0, (bool *)&am_exhaustive,
              "require exhaustive dependencies (makes --blind \"safe\")");
  no_argument("blind", 0, (bool *)&blind, "do not track dependencies");
  string_argument("dotfile", 0, &create_dotfile,
                  "create a dotfile to visualize dependencies", "DOTFILE");
  string_argument("makefile", 0, &create_makefile,
                  "create a makefile", "MAKEFILE");
  string_argument("tupfile", 0, &create_tupfile,
                  "create a tupfile", "TUPFILE");
  string_argument("script", 0, &create_script,
                  "create a build script", "SCRIPTFILE");
  string_argument("tar", 0, &create_tarball,
                  "create a tar archive", "TARNAME.tar[.gz]");
  string_argument_list("include-in-tar", 'i', &include_in_tar,
                       "include in tarball", "FILENAME");
  no_argument("version", 'V', (bool *)&show_version,
              "display the version");

  no_argument("dry", 0, (bool *)&dry_run,
              "dry run (don't do any building!)");
  string_argument("parse-only", 0, &parse_only,
                  "just parse this .fac file", "FACFILENAME");

  listset *cmd_line_args = NULL;
  {
    const char **extra_args = parse_arguments_return_extras(argv);
    char *cwd = getcwd(NULL,0);
    for (const char **arg = extra_args; *arg; arg++) {
      char *abspath = absolute_path(cwd, *arg);
      insert_to_listset(&cmd_line_args, abspath);
      free(abspath); // insert_to_listset makes a copy
    }
    free(cwd);
    free(extra_args);
  }

  if (show_version) {
    printf("fac version %s\n", version_identifier);
    exit(0);
  }

  root = go_to_git_top();

  if (parse_only) {
    struct all_targets all;
    init_all(&all);
    parse_only = absolute_path(root, parse_only);
    read_fac_file(&all, parse_only);
    printf("finished parsing file %s\n", parse_only);
    exit(0);
  }

  struct cmd_args args;
  args.include_in_tar = include_in_tar;
  args.create_dotfile = create_dotfile;
  args.create_makefile = create_makefile;
  args.create_tupfile = create_tupfile;
  args.create_script = create_script;
  args.create_tarball = create_tarball;
  args.strictness = normal;
  if (am_strict) {
    args.strictness = strict;
  }
  if (am_exhaustive) {
    args.strictness = exhaustive;
  }
  args.blind = blind;
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
