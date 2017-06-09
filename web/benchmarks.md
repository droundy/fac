# Benchmarks

$docnav

We present here a large number of benchmarks.  You can click on each
plot to view that plot as a high-resolution SVG file.

<!-- Each test was done using multiple file systems, to see how sensitive -->
<!-- the result is to file system speed.  Also note that you can click on -->
<!-- each plot to view a high-resolution version of that plot. -->

One major conclusion is that `fac` is generally a factor of two or
three times slower than make.  But do keep in mind that `fac` does
a lot more than make does, and is considerably easier to use in a
robustly correct manner.  There are also still some remaining scaling
issues that should be addressed, which arise when there are 10k or so
files involved.

## Deep hierarchy

$\newcommand\O[1]{\mathcal{O}(#1)}$
In the following benchmarks, there are 9 C files and 9 header files
per directory, as well as 9 subdirectories.  Each C file imports 10
header files as well as its matching header file.  Every C file
implements the same simple linked list, and imports more system
headers than it needs (but not an unusual number) to try to make
compile times close to typical.  In all of the plots below $N$ is the
number of C files that need to be built.
<a href="hierarchy-building.svg"><img
src="hierarchy-building.svg" alt="build times"/></a>

### Initial build (hierarchy)

The initial build takes linear time ($\O{N}$) in the best-case
scenario.  Some build systems, however, have a large constant term
which can make a large difference up to surprisingly large systems.
In addition, some build systems may have a different $\O{N}$
prefactor, which can make an even bigger difference for large
systems. <a href="hierarchy-touching-all.svg"><img
src="hierarchy-touching-all.svg" alt="rebuild times"/></a>

### Touching all (hierarchy)

For the rebuild, we touch all the C files.  The cost is still $\O{N}$
at best, but by remembering a hash of file content it is possible to
dramatically reduce the cost of the rebuild, which allows scons to win
in this case by more than an order of magnitude.  <a
href="hierarchy-modifying-c.svg"><img src="hierarchy-modifying-c.svg"
alt="more build times"/></a>

### Modify a C file (hierarchy)

The following test modifies a single C file, adding a newline to its
end.  Thus in principle, the cost could be $\O{1}$, if you could
magically determine which files to rebuild.  However, if you need to
check modification times on every file, it will still be $\O{N}$.  I
believe tup "magically" determines which files to build by running a
background process that uses inotify (or similar) to wait for changes
to input files.  You can note that scons here has a dramatically more
expensive $\O{N}$ cost, as it reads each C file to check dependencies
(I think).  <a href="hierarchy-modifying-header.svg"><img
src="hierarchy-modifying-header.svg" alt="more build times"/></a>

### Modify a header file (hierarchy)

The following modifies a single header file, adding a newline to its
end.  This should be close to identical to the former test, but
requires rebuilding about 10 times as many files.  Thus the $\O{1}$
term is increased while the $\O{N}$ term is hardly affected.  <a
href="hierarchy-doing-nothing.svg"><img
src="hierarchy-doing-nothing.svg" alt="more build times"/></a>

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
<a href="dependent-chain-building.svg"><img
src="dependent-chain-building.svg" alt="build times"/></a>

### Initial build (linear chain)

Obviously again this must be $\O{N}$.  As usual, scons and tup have a
noticeable $\O{1}$ contribution, which is much more significant for
tup.  FIXME I think there must be a bug in the scons benchmarking in
this case.
<a href="dependent-chain-rebuilding.svg"><img
src="dependent-chain-rebuilding.svg" alt="rebuild times"/></a>

### Rebuild (linear chain)

Here we remove all the generated executables and rerun, so again it is
$\O{N}$.  This technically does not require an entire rebuild, since
we need do not need to rerun the executables, so long as they come out
the same as last time.  But that is irrelevant, because compiling and
linking is way slower than running the executables.
<a href="dependent-chain-modifying-c.svg"><img
src="dependent-chain-modifying-c.svg" alt="more build times"/></a>

### Modifying a C file (linear chain)

Here we modify a single C file, which causes the whole chain to be
rebuilt for most build systems, thus costing $\O{N}$.  Again, scons's
tracking of file checksums makes this much faster, as it realizes that
the output is identical, so there is no rebuilding required.  I do not
understand why tup is cheap here.  Tup should need to rebuild
everything.
<a href="dependent-chain-modifying-header.svg"><img
src="dependent-chain-modifying-header.svg" alt="more build times"/></a>

### Modifying a header (linear chain)

This test touches a header that only requires one rebuild.  It thus
requires only $\O{1}$ rebuilds, but should with most systems require
$\O{N}$ checks of file modification times.  Presumably my tests are
small enough that we aren't able to see the $\O{N}$ costs clearly.
<a href="dependent-chain-doing-nothing.svg"><img
src="dependent-chain-doing-nothing.svg" alt="more build times"/></a>

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
<a href="cat-building.svg"><img
src="cat-building.svg" alt="build times"/></a>

### Initial build of cats

Obviously again this must be $\O{N}$.  As usual, scons and tup have a
noticeable $\O{1}$ contribution, which is much more significant for
tup.
<a href="cat-rebuilding.svg"><img
src="cat-rebuilding.svg" alt="rebuild times"/></a>

### Rebuild cats

Here we modify the source file from which the entire dependency chain
depends, which results in $\O{N}$ run cost.  <a
href="cat-doing-nothing.svg"><img src="cat-doing-nothing.svg"
alt="more build times"/></a>

### Doing nothing to cats

This test involves no changes, and thus should be very fast, as is the
case for all the "do nothing" builds.

## A few slow builds (all sleeps)

The following is a highly artificial test to see if tools can build in
an optimal order.  This test involves a buch of commands which sleep
for a given amount of time before producing a file.  These commands
are in dependency chains 3 commands long, and three of these chains
are slow.  So a parallel build should start the slow builds
immediately to finish in an optimal amount of time.  Naturally, there
is no way to determine which builds are slow until the second try.

This test is designed to represent the case (which is common with my
research code) where a few rules take much more time than the others.
Thus, it is best to start the slow rules early, since the total build
time is determined by when we start those few rules.

<a href="sleepy-building.svg"><img
src="sleepy-building.svg" alt="build times"/></a>

### Initial build of sleeps

This must be $\O{N}$, and should come out to somewhere between $N$ and
$1.5N$ seconds.  The first build can be done a bit better first
building commands that are required in order to build other commands.
Make and fac both use this trick, and run a bit faster.  <a
href="sleepy-rebuilding.svg"><img src="sleepy-rebuilding.svg"
alt="rebuild times"/></a>

### Rebuild sleeps

At this stage we should be able to finish rebuilding in $N-9$ seconds,
which is faster than the initial build, if we pay attention to how
much time the builds took the first time around.  Fac does this, and
manages to beat the competition on this test.

## Independent C files

A bunch of C files with no interdependencies.  A sort of polar
opposite of the linearly dependent tests.

<a href="independent-building.svg"><img
src="independent-building.svg" alt="build times"/></a>

### Initial build of independent C files

This is quite clearly a linear scaling as with all initial builds,
although it demonstrates that fac has an unusually high constant
term, due presumably to hashing the information for all the input
files that are outside the repository (compiler, header files,
libraries, etc.).

<a href="independent-rebuilding.svg"><img
src="independent-rebuilding.svg" alt="build times"/></a>

### Re-build of independent C files

Here we delete all the output and rerun the build.  It looks similar
to the initial build to me.

<a href="independent-modifying-c.svg"><img
src="independent-modifying-c.svg" alt="build times"/></a>

### Modifying a single independent C file

We modify a single C file.  We would like this build to be $O(1)$, and
it looks pretty good, even though we need to check $O(N)$ input files
to see if they have been modified.  Tup, of course, scales beautifully
here, although it doesn't catch up with ninja, make, or a blind fac at
less than about 10,000 files.

<a href="independent-doing-nothing.svg"><img
src="independent-doing-nothing.svg" alt="build times"/></a>

### Nochanges to the independent C files

Here with no changes, again we would dream of $O(1)$ time, but that
can only be acheived if you have a daemon running to tell you that no
files were touched.  Or I suppose if you could use the modification
time of the directory to determine that no changes were made, but I
don't think that's correct.  Ninja is the clear winner here.
