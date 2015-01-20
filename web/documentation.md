# Documentation

## Bilge file format

To configure this build system, you create a file called `top.bilge`.
This file specifies the rules to build your project.  For most
moderately complex projects, your `top.bilge` file will only specify
rules needed to create one or more additional `.bilge` files, which
will contain the rules for doing the actual build.  Each `.bilge` file
consists of:

1. Comments beginning with `"# "` (a pound sign followed by a space).

2. Rules beginning with `"| "` (a pipe character followed by a
   space).  The remainder of the line is the actual command to perform
   the build.  Following this line (possibly separated by blank lines
   and comments) are one or more of the following directives.  The
   order of these directives has no effect, so long as the follow the
   `"| "` line to which they apply.

3. Output specifications beginning with `"> "` followed by the name of
   the file that is output.  There is no escaping performed, and only
   newlines and null characters are disallowed in the file name.

4. Input specifications beginning with `"< "` followed by the name of
   the file that is required.  You only need specify inputs when they
   are generated files.  Even then, you need only specify the inputs
   if you wish to have the build reliably succeed on the first attempt.

## Running noname

To run noname, you simply execute

    noname [options] [filenames]

with the following options.

--jobs=N, -jN
: Specify the number of jobs to run simultaneousy.  This defaults to
  the number of processors available on your computer.

--verbose, -v
: Provide extra debugging output.

--show-output, -V
: Show the output of every command (stdout and stderr), even if that
  command succeeds.

## To do list

1. Add option to output a Makefile for the build.

7. Add syntax for specifying a cache directory, which will be ignored
   for both outputs and inputs.  sass is an example that uses such a
   cache directory that is likely to be in the tree.

8. Change timing of commands to use elapsed wall-clock time rather
   than CPU time, to get better estimates.

9. Optimize `parallel_build_all` better by eliminating loops over
   targets where possible and reusing `rules` list.

- Kill jobs on SIGINT (attempted, but not quite working)
- Hashing of inputs
- Tracking of environment variables

- clean command?
- files to build on the command line

- Enable bilge to call bilge recursively (requires ptrace effort)

- Check if output files are outside the directory tree
- Create .gitignore files?

- Support for ~ as home directory? :(
