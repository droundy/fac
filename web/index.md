# Noname build system

This build system has several advantages.

* Automatically tracks build dependencies in a way that is independent
  of programming language.

* Parallel building.

* You are forced to write your configuration in a language of your own
  choice.  (Or conversely, you are not forced to use a language of
  *my* choice, much less a custom-built language that I developed.)

* Integrates with git, to keep you from forgetting to `git add` a file
  that is needed for the build.

## How does it work?

- Noname uses ptrace to track every system call your build command makes.
  Thus we can see precisely which files are read, and which files are
  modified.

- Noname has an extremely simple declarative
  [file format](documentation.html).  There are no variables, no
  functions, no macros.  Just data.  This could be a problem if you
  were forced to write these files by hand.  But in most cases you
  will write a script to generate these files.

- You write you "build" script as a program (in the language of your
  choice) that creates a `.bilge` file.  This script is run (and
  re-run) using the same dependency-tracking mechanism that is used to
  for an ordinary build.  Thus, you can get away with writing a simple
  but inefficient script, since it will only seldom be run.  (Unlike,
  e.g. `scons` which has to rerun your `SConstruct` python file on
  every build.)

- If your build rules depend on the operating system, or the system
  environment, your "configure" script is the same program (in the
  language of your choice) that creates a `.bilge` file.  Or perhaps
  it provides input to the script that actually creates the `.bilge`
  file.
