# Fac


<!-- [![Build Status](https://travis-ci.org/droundy/fac.svg?branch=master)](https://travis-ci.org/droundy/fac) -->

[![Build Status](https://travis-ci.org/droundy/fac.svg?branch=master)](https://travis-ci.org/droundy/fac)
[![Build status](https://ci.appveyor.com/api/projects/status/opg6nds3m9ahkqvj?svg=true)](https://ci.appveyor.com/project/droundy/fac)

Fac is a general-purpose build system inspired by make that utilizes
ptrace to ensure that all dependences are enumerated and that all
source files are added to a (git) repo.  An important feature of fac
is that it automatically handles dependencies, rather than either
complaining about them or giving an incorrect build.  Currently, fac
only runs on linux systems, but on those systems it is incredibly easy
to use!

* Fac automatically tracks build dependencies in a way that is
  independent of programming language.  You are only required to
  specify the minimum of dependencies for each rule, and fac works out
  the rest for you.  If you fail to specify dependencies, fac should
  still build successfully after enough tries, provided your build
  rules fail when dependencies are missing (rather than simply
  producing wrong output).  Once fac has successfully built your
  project, it knows the dependencies of each command, and subsequent
  builds in that source tree will be the same as if you had specified
  all dependencies and all output.

* Fac supports parallel builds.

* You are forced to write your configuration in a language of your own
  choice.  (Or conversely, you are not forced to use a language of
  *my* choice, much less a custom-built language that I developed.)

* Integrates with git, to keep you from forgetting to `git add` a file
  that is needed for the build.

To find out more about fac, including benchmarks and complete
documentation, please visit the fac web page at:

http://physics.oregonstate.edu/~roundyd/fac

## Build and install

The easy way to get fac is to first install rust on your computer,
and then to type

    cargo install fac

## Build and install from a git clone

To build fac (and its documentation) just run

    cargo build

This should build fac on an x86-64 linux system that has rust
installed.  You can then build an
optimized version by running

    target/debug/fac fac

To use fac, you can copy the fac binary into some location in your
path.

### Build dependencies and details

You need to have [rust installed](https://rustup.rs).  In addition,
building fac with fac itself requires both `python2` and `python3` (something to fix), and
building the fac documentation (which is the default build target)
requires `sass` and `python-markdown`.

For more detail on building fac, see the
[web page on building fac](http://physics.oregonstate.edu/~roundyd/fac/building.html),
which is also in the fac repository as `web/building.md`.

## License

Fac is free software, and is licensed under the GNU General Public
License, version 2 or later.
