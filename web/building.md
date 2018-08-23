# Building fac

$docnav

Before building fac, you need to [install rust](https://rustup.rs).
You will also need to install python3, which is used for various build
scripts.  On a Debian-based
system, you can install all of this with

    apt-get install rust python3

You can obtain the fac source code using git clone:

    git clone git://github.com/droundy/fac.git

To build fac (assuming you have just cloned fac, and do not have an
older version of fac) just run

    cargo build --release && cp target/release/fac .

This should build fac on an x86-64 or 32-bit x86 linux system.  You
can then rebuild fac by running if you care to.

    ./fac fac

This will also (if possible) build the documentation and a few other
peripherals.  To use fac, you can copy the fac binary into some
location in your path.

Building the documentation (by running `./fac`) will require a few
more packages: sass, python-markdown, and help2man.
