# Benchmarks

We present here a large number of benchmarks.

Have fun!

## Deep hierarchy

$\newcommand\O[1]{\mathcal{O}(#1)}$
In the following benchmarks, there are 9 C files and 9 header files
per directory, as well as 9 subdirectories.  Each C file imports 10
header files as well as its matching header file.  Every C file
implements the same simple linked list, and imports more system
headers than it needs (but not an unusual number) to try to make
compile times close to typical.  In all of the plots below $N$ is the
number of C files that need to be built.
<a href="hierarchical-building.pdf"><img
src="hierarchical-building.png" alt="build times"/></a>

### Initial build (hierarchy)

The initial build takes linear time ($\O{N}$) in the best-case
scenario.  Some build systems, however, have a large constant term
which can make a large difference up to surprisingly large systems.
<a href="hierarchical-rebuilding.pdf"><img
src="hierarchical-rebuilding.png" alt="rebuild times"/></a>

### Rebuild (hierarchy)

For the rebuild, we remove all the generated files.  The cost is still
$\O{N}$ at best, but by caching it is possible to
dramatically reduce the cost of the rebuild, which allows scons to
win this contest.
<a href="hierarchical-touching-c.pdf"><img
src="hierarchical-touching-c.png" alt="more build times"/></a>

### Touch a C file (hierarchy)

The following test modifies a single C file.  Thus in principle, the
cost could be $\O{1}$, if you could magically determine which files to
rebuild.  However, if you need to check modification times on every
file, it will still be $\O{N}$.  I believe tup "magically" determines
which files to build by running a background process that uses inotify
(or similar) to wait for changes to input files.  You can note that
scons here has a dramatically more expensive $\O{N}$ cost, as it reads
each C file to check dependencies (I think).
<a href="hierarchical-touching-header.pdf"><img
src="hierarchical-touching-header.png" alt="more build times"/></a>

### Touch a header file (hierarchy)

The following modifies a single header file.  This should be close to
identical to the former test, but requires rebuilding about 10 times
as many files.  Thus the $\O{1}$ term is increased while the $\O{N}$
term is hardly affected.
<a href="hierarchical-doing-nothing.pdf"><img
src="hierarchical-doing-nothing.png" alt="more build times"/></a>

### Doing nothing (hierarchy)

The cost of doing nothing can be $\O{1}$ if (as tup does) you employ a
background process to watch for changes.  Otherwise, it is necessarily
$\O{N}$ as one checks each file for modifications.

## Linear dependency chain, flat directory

In the following tests, we have $N$ C files,
arranged in a linear chain.  Each C file compiles to an executable
that is run to generate a header file that is included by the
following C file.  This tests the scaling of each build system in the
extreme case of a highly dependent build, in contrast to the previous
case, in which each build operation could be performed independently.
<a href="flat-dependent-building.pdf"><img
src="flat-dependent-building.png" alt="build times"/></a>

### Initial build (linear chain)

Obviously again this must be $\O{N}$.  As usual, scons and tup have a
noticeable $\O{1}$ contribution, which is much more significant for
tup.
<a href="flat-dependent-rebuilding.pdf"><img
src="flat-dependent-rebuilding.png" alt="rebuild times"/></a>

### Rebuild (linear chain)

Here we remove all the generated executables and rerun, so again it is
$\O{N}$.  Again, scons wins by caching, and the initial $\O{1}$ for
tup is a bit less, since its database has already been generated.
<a href="flat-dependent-touching-c.pdf"><img
src="flat-dependent-touching-c.png" alt="more build times"/></a>

### Touching a C file (linear chain)

Here we modify a single C file, which causes the whole chain to be
rebuilt for most build systems, thus costing $\O{N}$.  Again, scons's
tracking of file checksums makes this much faster, as it realizes that
the output is identical, so there is no rebuilding required.  I do not
understand why tup is cheap here.  Tup should need to rebuild
everything.
<a href="flat-dependent-touching-header.pdf"><img
src="flat-dependent-touching-header.png" alt="more build times"/></a>

### Touching a header (linear chain)

This test touches a header that only requires one rebuild.  It thus
requires only $\O{1}$ rebuilds, but should with most systems require
$\O{N}$ checks of file modification times.  Presumably my tests are
small enough that we aren't able to see the $\O{N}$ costs clearly.
<a href="flat-dependent-doing-nothing.pdf"><img
src="flat-dependent-doing-nothing.png" alt="more build times"/></a>

### Doing nothing (linear chain)

This test should be close to identical to the previous one, but
without the $\O{1}$ build cost.  Again, it looks boring because it
doesn't (yet) cover very large builds.

## Linear dependency chain, flat directory, cats

In the following tests, we have a very simple dependency chain in
which we `cat` a file $N$ times.  This tests the scaling of each build
system in the extreme case of a highly dependent build, but with an
even faster build command, to highlight scaling issues by making
$\O{N}$ costs as small as possible.
<a href="flat-cats-building.pdf"><img
src="flat-cats-building.png" alt="build times"/></a>

### Initial build of cats

Obviously again this must be $\O{N}$.  As usual, scons and tup have a
noticeable $\O{1}$ contribution, which is much more significant for
tup.
<a href="flat-cats-rebuilding.pdf"><img
src="flat-cats-rebuilding.png" alt="rebuild times"/></a>

### Rebuild cats

Here we remove all the generated executables and rerun, so again it is
$\O{N}$.  Again, scons wins by caching, and the initial $\O{1}$ for
tup is a bit less, since its database has already been generated.
<a href="flat-cats-doing-nothing.pdf"><img
src="flat-cats-doing-nothing.png" alt="more build times"/></a>

### Doing nothing to cats

This test should be close to identical to the previous one, but
without the $\O{1}$ build cost.  Again, it looks boring because it
doesn't (yet) cover very large builds.
