# Building fac

$docnav

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

    ./fac fac

To use fac, you can copy the fac binary into some location in your
path.

You may also wish to build the documentation (by running `./fac`), but
this will require a few more packages: sass, graphviz, and
python-markdown.


## Cross-compiling fac for windows

To attempt to cross-compile fac for windows, issue:

    CC=/usr/bin/x86_64-w64-mingw32-gcc fac

This requires that the libpopt.a and popt.h both reside in ../win32.
It also doesn't yet work, but will compile some of fac's source files.

