# Introducing the fac build system

For the last year and a half I have been working on a new build
system, which I call fac.  For over a year now I have been using fac
for essentially all my build-tool needs, and in fact have been
inflicting it on students working in my research group.  And I am now
finally getting around to announcing fac to the wider community.

I have been using build systems for a long time now, and I use them a
*lot*.  As a graduate student, I discovered that when I returned to an
older project, I would seldom remember how to build it, and learned
that make was a great way to document the build process.  But it isn't
just code: it is often way harder to remember how to build the plots
needed for an old paper.  So I tend to create a *lot* of build
scripts.  A quick count shows that in the last year (approximately), I
have created over 30 "projects" with fac build scripts, and this is
not unusual for me.  I build research code, class websites, fun
programming projects, papers, posters, talks, and my vita.  Many of
these projects are very simple, but many of them involve integrating
together several build steps often involving different tools
(e.g. creating figures for papers or web sites).

Make is a wonderful tool, but it can be tedious getting it to rebuild
at the right times, since you need to specify all the dependencies by
hand.  A few years back I switched over to [scons](http://scons.org).
Scons can track dependencies for you, but needs to know about each
type of input file.  You can tell scons how to build your project, but
that involves writing fiddly parsing code for each sort of input file,
or giving up and manually listing dependencies.  On top of which,
scons is incredibly slow.  In frustration, I tried a build tool called
[tup](http://gittup.org/tup/), which claimed to be incredibly fast.
Tup automatically tracks build dependencies by using a fuse filesystem
to intercept file system reads and writes.  However, it had other
issues (most painfully, it requires the suid `fusermount` program,
which interacts poorly with NFS and rootsquash).  Eventually, I
decided surely I could do better! (Hey, the progess of humanity relies
on people who do stupid things!)

## Enter fac

Okay, enough autobiography! What is this tool, you ask, and why would
I want to use it? Excellent question, dear reader!

Fac is a general-purpose build tool (in contrast with
cargo, gradle, ant, etc, which are designed around building programs
written in a specific language) that tracks dependencies for you, and
requires very little effort to teach to build your code, while
still allowing you to build code that requires configuration of
arbitrary complexity.

How do we manage this simplicity with complexity? The key is that fac
does *not* provide you with *any* programming-language features.
There are no variables, no control structures, no functions, no
macros, and not even any string escaping!  I presume that you already
have one or more programming languages you are happy with, and allow
you to use one of them to configure fac (if programming is required).

A .fac file (which is a file that ends with .fac) consists of
single-line expressions that describe your build.  Each of these lines
begins with a character indicating what it means, followed by a space
(for human legibility), followed by the relevant (unescaped) data.
This does mean that fac will not support file names containing
embedded newlines.  A simple .fac file would look like:

    | cat input > output
    > output
    < input

This tells fac that it should execute the command "cat input >
output", which will require the file "input" and will create the file
"output".  In this case, you would generally omit all but the first
line, since fac can determine the inputs and outputs for itself.  In
fact, the only reasons to specify inputs or outputs are

1. You may want to specify generated input files, so that fac can
   build the project in the correct dependency order the very first
   time.  If you omit to specify generated inputs, so long as your
   build commands will fail on missing input, you can simply run fac a
   few times to get things built.

2. You may want to specify outputs so you can ask fac to build just
   one file (out of many), which it has never built before.  Or you
   may want to document for the readers of your .fac file what a given
   command is expected to produce.

3. Your rule generates a new .fac file that should be processed.  See
   the following section!

## Adding configuration

It isn't uncommon that your build tool needs to behave differently
based on the environment.  A classic example is needing to set the
compiler flags based on which version of the compiler is present.

Fac handles this case by allowing you to specify rules that generate
new .fac files.  This rule can be anything (and can depend on building
other rules), but *must* have its output specified to fac.  Consider
the following (extremely stupid) example .fac file:

    | echo '| cat input > output' > output.fac

This tells fac to generate the output.fac and then read that file,
which will tell it how to generate output from input.  I typically use
python to write these little scripts, but any language will do.

The strenth of this approach is that fac will only rerun these
configuration steps if their *inputs* change.  The same logic (and
dependency tracking) that applies to an ordinary build applies to the
configuration step.  So you can go ahead and boldly introduce
complexity to your build without suffering from a slow build *or* from
the possiblity of forgetting to rerun the configure step.

## Git integration

For years I struggled to keep git hooks working to test each commit of
my research code.  Eventually, I realized that really the main benefit
of that work was simply to avoid forgetting to add new files to the
repository.  It's *so* frustrating to `git pull` only to discover that
the build is now broken.

Fac fixes this problem in a nice way, that doesn't add the extra two
hours that it takes to build my research code.  (That is another
story...)  Fac will fail a build, if it uses an input file that is in
your source code directory, is not in the git repository, but is not
generated by another rule.  This means that you cannot forget to `git
add` a new file, which is really all I needed from my painfully slow
git hooks.

## Generating Makefiles

Another little trick fac has up its sleeve is the ability to generate
a Makefile for you (or a build shell script).  This is how you can
bootstrap fac on your own machine:  it comes with a shell script (and
a redundant makefile) that is generated by fac itself.  This isn't
terribly useful, but may be comforting, if you are afraid what will
happen if you choose to stop using fac.

## How does it work?

Fac uses the ptrace system call to track all system calls that a build
rule uses.  It recognizes those calls that read or write to files or
directories.  Really, that's all.  It wasn't that easy to program, but
it's not all that subtle, either.  This does add a bit of overhead,
but in most tests for real builds, it doesn't seem particularly
significant.  In order to track whether inputs have changed, fac
computes hashes of each input, which does add a bit more overhead, but
still (I think) at an acceptable level.  Another side-effect is that
you may run into trouble if your build rules themselves use ptrace.
I'm not sure about that.

Sadly, this approach isn't portable to either MacOS or Windows.  There
are similar approaches that could work, but given that I don't own a
machine running either of those operating systems, I haven't had a
chance to attempt porting fac.  I did put some effort into porting to
freebsd using ktrace, with the hope that this would trasfer over to
MacOS, which is based on freebsd.  But then I discovered that the
ktrace system call had been eliminated from MacOS.

## What next?

I hope that some of you will find fac intriguing.  It is very much
usable for every day work... as I use it for my work every day.  It
has various other features that you can read about on the
[fac website](http://physics.oregonstate.edu/~roundyd/fac).  If you
have questions or comments, please send them to the
[fac mailing list](mailto:fac-users@googlegroups.com).

## Who am I?

I am David Roundy, an Associate Professor in the Department of Physics
at Oregon State University.  Among programming folks, I am best known
for having created the
[darcs revision control system](http://darcs.net).  You can find out
more about my research and teaching by visiting
[my web page](http://physics.oregonstate.edu/~roundyd).
