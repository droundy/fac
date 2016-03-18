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
    Build succeeded! 0:0...

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
has been generated.

    $ fac
    1/2 [...s]: sha1sum ...
    2/2 [...s]: pdflatex ...
    Build succeeded! 0:0...

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
    Build succeeded! 0:...

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
    error: missing file typo.tex, which is required for paper.pdf
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
    error: add typo.tex to git, which is required for paper.pdf
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

