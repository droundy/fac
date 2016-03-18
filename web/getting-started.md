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
it list the files in our repository! Here is a small change to our
latex file to include the file `files.tex`.

##### paper.tex
    \documentclass{article}
    \begin{document}
    We have \input{files}
    \end{document}

Now we just need to create a rule that will generate our `files.tex`.

##### build.fac
    | pdflatex paper.tex
    
    | sha1sum paper.tex > files.tex

[run]: # (foo)

