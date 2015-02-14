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
confusing if you expect to use environment variables to set the path,
for instance.

**Make** and **tup** will achieve a minimal rebuild only provided the
ordering of file modification times is sufficient to determine what
requires rebuilding.

## Correct build

**Fac** should always give you a correct build (if it is successful),
meaning that every file that needs to be rebuilt will be rebuilt.  Fac
can achieve this even if you provide no dependency information, or
wrong dependency information, although you may need to run fac many
times in such a case.  The other case where you could end up with an
incorrect build is if you modify a file in a way that does not change
its modification time or its file size.

**Tup** will almost always give you a correct build, with the exceptions
being if an out-of-tree file was modified (e.g. you upgraded a
library), or if you change an environment variable that affects your
build.

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
  output when fed missing input without failing) is a need to run fac
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
  have this very important feature.
