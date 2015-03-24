# Features

This page summarizes the features of fac, and compares these features
with make, scons, and tup, the three build systems with which I am
most familiar.

## Minimal rebuild

**Fac** enables you to have a truly minimal rebuild, with very a few
exceptions.  One exception is that if you change an environment
variable, you will trigger an entire rebuild, which may not be needed.

**SCons** is similar in this regard, but scons always computes the
hash of every input file, even if its size and modification time are
unaltered.  Also, scons has a different approach to environment
variables, which involves shielding the build from your environment.
This avoids the spurious rebuild issue that fac has, but can be
confusing if you expect to use environment variables to set the path.

**Tup** will achieve a minimal rebuild if the ordering of file
modification times provides sufficient information in order to do so.
e.g. tup will not give a minimal build if you make an unimportant
change in one file that may trigger a chain of rebuilds.

**Make** will achieve a minimal rebuild provided the ordering of file
modification times is sufficient to determine what requires
rebuilding, and that you have exhaustively and non-conservatively
enumerated every dependency.

## Correct build

**Fac** should always give you a correct build (if it is successful),
meaning that every file that needs to be rebuilt will be rebuilt.  Fac
can achieve this even if you provide no dependency information, or
wrong dependency information, although you may need to run fac many
times in such a case.  You could also end up with an incorrect build
if you modify a file in a way that does not change its modification
time or its file size.  And finally, if you edit a file while it is
being used in a build, fac can fail to rerun the build.

**Tup** will almost always give you a correct build, with the
exceptions being if an out-of-tree file was modified (e.g. you
upgraded a library).  Tup can track out-of-tree files, but only if you
install it as suid-root (see security issues below).

**Make** will give you a correct build only if you annotate all the
dependencies for your build correctly, and only if the stars align.
If you edit your makefile, change environment variables, have issues
with your clock, etc, your build will not be correct.

**SCons** puts in some effort towards getting a correct build.  It
likely succeeds, provided it is either able to figure out all the
dependencies correctly, or you specify all dependencies manually.  It
does not look outside the source tree for dependencies, so if you
install a new version of a library, you will need to `scons -c` (I
believe).

## Git integration

**Fac** forces you to add your sources to your git repository.  I see
  this as a major advantage, as it eliminates the bug where you forget
  to git add a file, and thus break the build.  It could also be
  annoying, depending on your attitude.  So far as I am aware, this
  feature is unique.

## Sloppy build scripts

**Fac** allows for a very sloppy build configuration.  It is picky
  about adding sources to git, but not about specifying all inputs.
  The worst you should suffer (unless your build rule gives wrong
  output without failing when missing input) is a need to run fac
  a few times.  You do not, however, lack any of the other features
  listed on this page.

**Tup** is at the opposite extreme.  You must list every output file
  from every command, and any inputs that are automatically
  generated.  The fact that tup will tell you what is missing helps a
  bit, but this can be painful if the outputs differ from system to
  system.  But on the plus side, sloppy scripts will cause tup to fail
  with a nice message rather than giving wrong output.

**Make** and **SCons** both will happily give wrong output if you give
  them a configuration that is missing inputs.  See
  [a horror story](fac-vs-scons.html) for how wrong a scons build can
  go if you enable caching.

## Parallel builds

**Everything** gives you parallel builds, including **fac**.  But I
  didn't want to leave it off the list, lest you think fac does not
  have this important feature.

## Progress notification

**Fac** (like **tup**) provides estimates of the build time remaining
  as you perform your build.  These estimates make use of the time
  spent on each rule during previous builds, and thus for rebuilds the
  estimates can be quite accurate.

## Clear build output

When performing a parallel build, **fac** shows the output of each
build (by default only the failing builds) separately, right after the
build command itself is printed.  This is in stark contrast to
**make** and **scons**, which send the output from each running
command directly to the terminal, which can lead to a confusing and
useless mess of information.  The downside is that fac does not
provide up-to-date output to the terminal.  To see output as it is
created by a particularly slow command, you can use the `--log-output`
option, and examine the log file as it is created.

## Three characters with one hand

**Fac** is unique among build systems in that it only requires you to
  type three characters ("f", "a", and "c") using one hand.  **Tup**
  also only requires three key presses, but on a standard QWERTY
  keyboard, with fingers in standard position, requires you to use
  both hands.  **Make** and **scons** are clearly inferior solutions,
  in that they each require you to hit *four* separate keys in order
  to invoke them.

## Security

**Fac** doesn't do anything that could violate the security of your
  computer.  This is a pretty low bar for a build system, since it is
  your own fault if you try to build malicious code.  **Make** and
  **scons** (and almost every other build system) also pass this bar.

**Tup**, however, presents a security risk.  Tup requires that your
  build be run by a user in the `fuse` group (at least on some linux
  distributions).  Furthermore, tup requires the use of the suid-root
  program `fusermount`, which can cause problems if you try to build
  in a directory that is nfs-mounted with rootsquash (the more secure
  default) that is not world-readable.  Finally, if you want to use
  tup to build with `gcc --coverage` (or any build step that makes use
  of absolute paths), tup needs to be installed as suid-root so that
  it can create a chroot.  This introduces the possibility of
  priviledge-escalation bugs.  It may be that namespaces will remove
  this requirement in the future, but they have already been shown
  (once) to introduce a security vulnerability (which was then fixed).
  Sticking with running everything as the user running the build seems
  safest.

## Stubborn

**Fac** will keep trying to build what it can, even after one rule
  fails...
