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

--clean, -c
: Clean up build output.  This deletes every file (but not directory)
  that is output by the build.

--verbose, -v
: Provide extra debugging output.

--show-output, -V
: Show the output of every command (stdout and stderr), even if that
  command succeeds.

## To do list

1. Add option to output a Makefile for the build.

2. Teach the benchmarking scripts to save their data in text files and
   separate out plotting.  Thus the (small) files could be added to
   the git repository.  This could also enable creation of (slightly
   arbitrary) quantitative benchmark metrics through averaging.

3. Add continuous build mode, in which we monitor input files for
   changes and rebuild as needed.  Ideally we would use inotify or
   similar to handle this with minimal CPU overhead.

4. Experment with a reverse tree, which would enable us to quickly
   identify rules that need to be rebuilt due to a changed input.
   This is especially appealing for the continuous build mode.

5. On BSD and Darwin systems, use ktrace rather than ptrace.

7. Add syntax for specifying a cache directory, which will be ignored
   for both outputs and inputs.  sass is an example that uses such a
   cache directory that is likely to be in the tree.

9. Optimize `parallel_build_all` better by eliminating loops over
   targets where possible and reusing `rules` list.

- Kill jobs on SIGINT (attempted, but not quite working)
- Hashing of inputs
- Tracking of environment variables

- files to build on the command line

- Enable bilge to call bilge recursively (requires ptrace effort)

- Check if output files are outside the directory tree
- Create .gitignore files?

- Support for ~ as home directory? :(
