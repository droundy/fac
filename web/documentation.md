# Documentation

## Building fac

Before building fac, you need to install its prerequisites.  This
consists of a C compiler (either gcc or clang), python2, python3, and
`libpopt` (which we use for parsing arguments).  On a Debian-based
system, you can install all of this with

    apt-get install build-essential libpopt-dev python python3

(Yes, it is a little silly using both python2 and python3 in the build
process... I should probably switch entirely to python3.)

You can obtain the fac source code using git clone:

    git clone git://github.com/droundy/fac.git

To build fac (assuming you have just cloned fac, and do not have an
older version of fac) just run

    sh build-linux.sh

This should build fac on an x86-64 or 32-bit x86 linux system.  You
can use `build-freebsd.sh` to build on freebsd.  You can then build an
optimized version of fac by running

    ./fac

To use fac, you can copy the fac binary into some location in your
path.

## Fac file format

To configure this build system, you create one or more files ending
with `.fac`.  These files specify the rules to build your project,
and must be added to your git repository.  For most moderately complex
projects, you will have just one `.fac` file in git, which will
itself specify specify rules needed to create one or more additional
`.fac` files, which will contain the rules for doing the actual
build.  Each `.fac` file consists of:

1. Comments beginning with `"# "` (a pound sign followed by a space).

2. Rules beginning with `"| "` (a pipe character followed by a
   space).  The remainder of the line is the actual command to perform
   the build.  Following this line (possibly separated by blank lines
   and comments) are one or more of the following directives.  The
   order of these directives has no effect, so long as the follow the
   `"| "` line to which they apply.

2. Optional rules beginning with `"? "` are identical to rules
   beginning with a pipe, with the sole difference being that optional
   rules are only built if they are needed to build another target, or
   if they are explicitly requested.  Optional rules enable you to
   specify a large number of rules and only have what is needed built.

3. Output specifications beginning with `"> "` followed by the name of
   the file that is output.  There is no escaping performed, and only
   newlines and null characters are disallowed in the file name.
   There is little need to specify the output for rules, since fac
   can determine this automatically.  The only reason to specify
   output is so that on the very first build a user can request that
   we build only the specified rule.

4. Input specifications beginning with `"< "` followed by the name of
   the file that is required.  You only need specify inputs when they
   are generated files.  Even then, you need only specify the inputs
   if you wish to have the build succeed on the first attempt.

5. There are two ways to specify "cache" files.  A cache file is a
   file that may be either read or written by a given command, but
   doesn't affect the output and not itself an output.  One nice
   example is the ".pyc" files sometimes generated when you run a
   python program.  One python command may produce a .pyc file, and
   another may read it (if they both use the same module), but that
   does not mean that there is a dependency between those two
   commands.  You can specify a cache suffix (such as `.pyc`) using a
   `"c "` line, or you can specify a cache prefix (such as
   `/home/droundy/.ccache`) using a capitalized `"C "` line.  The
   latter can be helpful if you get error messages stating that your
   rule is creating output outside your source tree.

## Running fac

To run fac, you simply execute

    fac [options] [filenames]

with the following options.

`--version`
: Display the version number of fac.

`--jobs=N, -jN`
: Specify the number of jobs to run simultaneousy.  This defaults to
  the number of processors available on your computer.

`--continual`
: Keep rebuilding whenever the source is modified.

`--clean, -c`
: Clean up build output.  This deletes every file (but not directory)
  that is output by the build.

`--verbose, -v`
: Provide extra debugging output.

`--show-output, -V`
: Show the output of every command (stdout and stderr), even if that
  command succeeds.

`--log-output LOG_DIRECTORY`
: Save the output of every command (both stdout and stderr) to a
  separate file in the directory `LOG_DIRECTORY`, which will be created if
  it does not yet exist.

`--makefile MAKEFILE`
: After building, create a makefile with name MAKEFILE, which can be
  used to perform this build if fac is unavailable.

`--script BUILD.SH`
: After building, create a shell script with name BUILD.SH, which can
  be used to perform this build if fac is unavailable.

`--tupfile TUPFILE`
: After building, create a tupfile, which can be used to perform this
  build if fac is unavailable.

## To do list

1. Make sure when we create a fd that we do so in the "primary" pid
   when threads are involved.

1. Move build process to use only python3 and not python2.

1. Look into using libseccomp to optimize the ptrace usage by having
   only the system calls we want to trace generate events.

2. Handle nonexistent files as dependencies, if a process tried to
   open them or stat them.

3. Port to darwin/macos.

6. Support for ~ as home directory? :(

## See also

* [Summary of features](features.html)

* [Documentation of the code](code-guide.html)

* [Signatures and git](signatures.html)
