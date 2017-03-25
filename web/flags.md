# Running fac

$docnav

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

`--git-add`
: Run `git add -- PATH` on any files that fac determines are needed
  for the build.  This is naturally a somewhat risky maneauver, but
  can be convenient if you know you have created a bunch of new input
  files.

`--strict`
: Make fac insist that the facfile specifies sufficient inputs to the
  build to determine a correct build order.

`--exhaustive`
: Make fac insist that *every* input for every rule is specified.

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
