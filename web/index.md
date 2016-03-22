# Fac build system

This build system has several advantages.

<img src="kells-fac.svg" alt="Fac"/>

* Automatically tracks build dependencies in a way that is independent
  of programming language.  You are only required to specify the
  minimum of dependencies for each rule, and fac works out the rest
  for you.  If you fail to specify dependencies, fac should still
  build successfully if you use it repeatedly.

* Parallel building.

* You are forced to write your configuration in a language of your own
  choice.  (Or conversely, you are not forced to use a language of
  *my* choice, much less a custom-built language that I developed.)

* Integrates with git, to keep you from forgetting to `git add` a file
  that is needed for the build.

## How does it work?

- Fac uses ptrace to track every system call your build command makes.
  Thus we can see precisely which files are read, and which files are
  modified.

- Fac has an extremely simple declarative
  [file format](documentation.html).  There are no variables, no
  functions, no macros.  Just data.  This could be a problem for
  larger projects if you were forced to write these files by hand.
  But in most large projects you will just write a script to generate
  these files.

- You write your "build" script as a program (in the language of your
  choice) that creates a `.fac` file.  This script is run (and
  re-run) using the same dependency-tracking mechanism that is used to
  for an ordinary build.  Thus, you can get away with writing a simple
  but inefficient script, since it will only seldom be run.  (Unlike,
  e.g. `scons` which has to rerun your `SConstruct` python file on
  every build.)

- If your build rules depend on the operating system, or the system
  environment, your "configure" script is the same program (in the
  language of your choice) that creates a `.fac` file.  Or perhaps
  it provides input to the script that actually creates the `.fac`
  file.
