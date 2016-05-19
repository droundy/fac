# Fac

Fac is a general-purpose build system
(i.e. make/automake/cmake/scons/etc. replacement) that tracks
dependencies automatically.  Currently, fac only runs on linux
systems, but on those systems it is incredibly easy to use!

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

To build fac (assuming you have just cloned fac, and do not have an
older version of fac) just run

    sh build-linux.sh

This should build fac on an x86-64 linux system.  You may be able to
use build-freebsd.sh to build on freebsd (but it is likely
bit-rotted).  You can then build an optimized version by running

    ./fac fac

To use fac, you can copy the fac binary into some location in your
path.

### Build dependencies and details

The most rare build dependency for fac is libpopt, which is included
in the `libpopt-dev` package in Debian-based distributions.  In
addition, fac requires both `python2` and `python3` (something to
fix), and building the fac documentation (which is the default build
target) requires `sass` and `python-markdown`.

For more detail on building fac, see the
[web page on building fac](http://physics.oregonstate.edu/~roundyd/fac/building.html),
which is also in the fac repository as `web/building.md`.
