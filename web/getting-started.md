# Getting started

$docnav

Using fac can range from trivial, for simple builds, to requiring
writing your own scripts to customize the build for different
environments.  This tutorial will step you through this process,
starting with a very simple script, and ending with a dynamically
configured build.

## Just one latex file

To begin with, let's start with a very simple latex file that we wish
to build.

##### paper.tex
    \documentclass{article}
    \begin{document}
    Greetings!
    \end{document}

We need to create a git repository and add this file to it.

    $ git init
    Initialized empty Git repository in ...
    $ git add paper.tex

Now to run fac, we will need to create a file ending in `.fac`.  Let's
just call this file `build.fac` for simplicity.  The fac file will
contain a single line that describes how to build the paper.

##### build.fac
    | pdflatex paper.tex

Now we can run the build using

    $ git add build.fac
    $ fac
    1/1 [...s]: pdflatex paper.tex
    Build succeeded! (0...

Note that `.fac` files need to be added to git repository before fac
will use them.  As you can see, fac reports on each command run, as
well as telling you the total time spent.  Of course, this is not yet
an interesting build.  However, if you are curious, you could peak at
the file `build.fac.tum`, which stores fac's knowledge about what it
has built.  In this case, it will list probably around 64 input files
and directories, along with their time stamps, sizes, and a hash of
their contents.  If any of these change, then fac will rerun the
command when you next run it.

## Adding a dependency

Now let us try to add some interest to our latex file.  We will make
it include the SHA1 hash of its contents. Here is a small change to
our latex file to include the file `hash.tex`.

##### paper.tex
    \documentclass{article}
    \begin{document}
    We have \input{hash}
    \end{document}

Now we just need to create a rule that will generate our `hash.tex`.

##### build.fac
    | pdflatex paper.tex
    
    | sha1sum paper.tex > hash.tex

At this point we have not told fac about any dependency between these
two commands.  This means that fac may fail the first time it is run,
since it could try running `pdflatex` on the paper before `hash.tex`
has been generated.  After the very first run (or the first run after
a new dependency is introduced between commands) the build should
always be correct with a single run of fac.

    $ fac
    1/2 [...s]: sha1sum ...
    2/2 [...s]: pdflatex ...
    Build succeeded! ...

Once you have run fac once (even if it fails), then the build will
succeed the second time, and from then on will know that it must
update the `hash.tex` before running pdflatex.

If we wanted to explicitly mark this dependency so that we can
guarantee that fac will succeed on the first run, we can do so by
adding a `<` line specifying the required input.

##### build.fac
    | pdflatex paper.tex
    < hash.tex
    
    | sha1sum paper.tex > hash.tex

which we can run with:

    $ fac
    Build succeeded! ...

Note that there is no need to specify the output of `sha1sum`.  Fac
will run any commands that it is able to run, and will only run
`pdflatex` after `hash.tex` has been generated.  If you were to
introduce a typo in the dependency...


##### build.fac
    | pdflatex paper.tex
    < typo.tex
    
    | sha1sum paper.tex > hash.tex

... fac will now refuse to run `pdflatex` with a polite error message.

    $ ! fac
    error: missing file "typo.tex", which is required for ...
    Build failed 1/1 failures

Now we can change paper.tex to actually depend on `typo.tex`, and
create such a file.

##### paper.tex
    \documentclass{article}
    \begin{document}
    We have \input{hash}.  And need typo file \input{typo.tex}
    \end{document}

##### typo.tex
    ``typo.tex''

At this point you might feel like things should work.  Our `build.fac`
file reflects an actual dependency, and all is good.  Not so, we
forgot a very important step!

    $ ! fac
    1/2 [...s]: sha1sum paper.tex > hash.tex
    error: add "typo.tex" to git, which is required for ...
    Build failed 1/2 failures

It is important for you to add your input files to git, or other users
will find that they are unable to build your project.  Fac checks
this, and ensures that you won't accidentally forget an important
file.

    $ git add typo.tex
    $ fac
    1/1 [...s]: pdflatex paper.tex
    Build succeeded! ...



[run]: # (foo)

## Programming the build

Fac does not have any programming language features in its file
format.  No variables, no macros, no functions, no lambda expressions.
Just data.  So what do you do when you have a complicated build? You
get to pick your favorite programming language with which to configure
fac, but generating a file ending in `.fac`.  In this example, I will
use python.  Let's say we want to build one program for each word
starting with the letter "h" in `/usr/share/dict/words`.  We clearly
don't want to enumerate each of these files (for me, over three
thousand rules)!

First, let's write a little program to generate the source code for
the example.  I am going to keep this script distinct from the code to
configure fac, since usually you probably write your source code by
hand.

##### build-source.py
    import re
    badchars = re.compile(r"[\"' \t]")
    with open('/usr/share/dict/words') as words:
        for word in words:
            word = word.strip()
            if len(badchars.findall(word)) == 0 and word[0:2] == 'he':
                with open('hello-%s.c' % word, 'w') as f:
                    f.write(r"""
    #include <stdio.h>
    void main() {
        printf("Hello %s!\n");
    }
                    """ % word)

Now let's edit our `build.fac` file to run this script.

##### build.fac
    | pdflatex paper.tex
    < typo.tex
    
    | sha1sum paper.tex > hash.tex
    
    | python2 build-source.py

We could run fac now to generate all this code, but let's wait a
moment until after we've configured it, so you can see how fac will
respond when given a somewhat trickier task.  We can now write the
program to tell fac how to compile all these files.  It is just a
python script that looks at all the `*.c` files in the current
directory and writes to stdout.

##### configure.py
    import glob
    for f in glob.glob('*.c'):
        print '| gcc -o %s %s\n' % (f[:-2], f)

Naturally, we could add more trickiness, like checking whether the
`$CC` environment variable is defined, `$CFLAGS`, etc.  But I'm
keeping this simple for now.  Now let's tell fac to run this script
also.

##### build.fac
    | pdflatex paper.tex
    < typo.tex
    
    | sha1sum paper.tex > hash.tex
    
    | python2 build-source.py
    
    * python2 configure.py > cfiles.fac
    > cfiles.fac

Note that it is very important that we informed fac that this command
will create a `.fac` file (`cfiles.fac`, in this case) by using a `>`
directive.  This tells fac that it should run this command as soon as
possible, *and* that it should read the generated `.fac` file as
further input.  We also informed fac that this command may need to be
rerun if directory contents change by defining it using a `*` rule.
Also note that we used a shell redirection.  Each
command in fac is run through the shell, so you can use any "shell"
trickery you care for.

We could run fac now to generate all this code, but let's wait a
moment until after we've configured it, so you can see how fac will
respond when given a somewhat trickier task.  We can now write the
program to tell fac how to compile all these files.  It is just a
python script that looks at all the `*.c` files in the current
directory and writes to stdout.

Now we just need to add these new sources to git and run fac.  (And
wait patiently as we build an incredible number of extremely pointless
C programs.)

    $ git add *.py
    $ fac
    ...: python2 configure.py > cfiles.fac
    ...: python2 build-source.py
    ...

At this point, fac will have created all the `.c` files, and may have
compiled some of them, depending on how many were generated before
`configure.py` was run.  If we wanted the build to be reliable the
first time, we would just have to specify that `build-source.py`
generates `hello-hello.c`, and that `configure.py` requires this (or
you could pick any other of the generated files).  Since fac probably
missed some (or all) of these the first time, let's run it again.

    $ fac
    1/1 [...s]: python2 configure.py > cfiles.fac
    2/... [...s]: gcc -o hello-he...
    3/... [...s]: gcc -o hello-he...
    ...

You can see that fac immediately reruns the configuration script, even
though it already ran it! This is because fac recognizes that because
new files have been created in this directory the configuration script
might give different output.  Fac cannot tell that only files endingin
`.c` will matter, but plays it safe since you have created new files.
Note that because fac runs the configuration as well as the build, you
cannot forget to run `./configure`.  Also, the configure step will
*only* be rerun when it might give different output (based on what fac
can tell), and will *always* be rerun if it *will* create different
output.

You have probably also noticed that fac tries to estimate the build
time remaining.  It likely did a poor job this time, but if you make a
minor change to `build-source.py` and rerun fac, you should see
reasonable estimates, since the time estimate for each command is
based on how long it took the last time it was run.  You may also have
noticed that by default fac runs builds in parallel, with the number
of parallel jobs equal to the number of cores (or possibly
hyperthreads).

I will point out here that if we had made `configure.py` generate the
C files as well as do the fac configuration, things would have built
right the first time, and  `configure.py` would have to be rerun much
more seldom (only when `/usr/share/dict/words` changes, or the python
ecosystem, or the script itself).

There are a few more subtleties to using fac (specifically dealing
with cache files, which are actually neither output nor input, but
could look to fac like either or both), but at this point you should
know enough to get started using fac.
