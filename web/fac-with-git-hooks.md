# Triggering fac with git hooks

$docnav

This tutorial shares how to use fac with both `fac --continual` and
git hooks to have your project automatically built and tested in a
very flexible way without the trouble of ever explicitly calling fac.

## Source code

As an example, let's pick a simple C program, but we will add
infrastructure towards making it more complicated.

##### hello.c
    #include <stdio.h>
    
    main() {
      printf("Hello world!\n");
    }

##### .clang-tidy
    Checks: '*'
    WarningsAsErrors: '*'
    HeaderFilterRegex: '.*'

We will create a git repository and add this file to it.

    $ git init
    Initialized empty Git repository in ...
    $ git config user.email "testing@example.com"
    $ git config user.name "Tester"
    $ git add hello.c .clang-tidy

Now let's get started with figuring out how to build this.  Although
we have only one C file, we anticipate creating a large program, so we
don't want to list all the C files manually.  Instead we will write a
program that generates a facfile called `.fac` that explains how to
compile everything.  For a real program, I would put in more
intelligence here to check what compilers are available, and what
flags are appropriate for said compilers, but let's keep this simple.

##### configure.py
    import glob, os
    clang_database = open('compile_commands.json', 'w')
    clang_database.write('[\n')
    facfile = open('.fac', 'w')
    objects = []
    sparses = []
    for c in glob.glob('*.c'):
        cmd = 'gcc -c %s' % c
        objects.append(c[:-1]+'o')
        sparses.append(c[:-1]+'sparse')
        clang_database.write('''{"directory": "%s",
                                 "command": "%s",
                                 "file": "%s"}
                             ''' % (os.getcwd(), cmd, c))
        facfile.write('| %s\n> %s\n' % (cmd, objects[-1]))
        facfile.write('? sparse -Wsparse-error %s > %s\n> %s\n' % (c, sparses[-1], sparses[-1]))
    clang_database.write(']\n')
    facfile.write('| gcc -o hello ' + ' '.join(objects) + '\n> hello\n')
    for o in objects:
        facfile.write('< %s\n' % o)
    facfile.write('? cat %s > sparse.log\n> sparse.log\n' % ' '.join(sparses))
    for o in sparses:
        facfile.write('< %s\n' % o)

This program also generates a clang compile database, which would be
helpful if we were using the clang tooling (which we aren't due to our
CI infrastructure not supporting clang-3.9).  In our example, we will
instead use a git version of sparse (which supports `-Wsparse-error`
to check our code.

    $ sparse --version
    v0.5.0-...

##### configure.fac
    | python3 configure.py
    > .fac

You might wonder, why all the trickiness with the `sparse` rules?  The
optional aspect `?` is because I don't expect everyone compiling the
program to have the git version of `sparse`, but want them to not have
to type `fac hello` to get a successful build.  The `cat` of many (in
our case one) `*.sparse` files into `sparse.log` is so we can have a
single fac target that runs sparse on any source files that need it
rerun (but no more).

Now we can just build the executable without running the by calling
`fac` with no arguments.

    $ git add configure.fac configure.py
    $ git commit -am 'first version'
    ...
    $ fac
    1/1 [...s]: python3 configure.py
    2/3 [...s]: gcc -c hello.c
    3/3 [...s]: gcc -o hello hello.o
    ...
    Build succeeded! ...

Or we can just run the lint (sparse) by

    $ fac
    ...
    $ fac sparse.log # fails
    hello.c:3:6: error: non-ANSI function declaration of function 'main'
    1/2 [...s]: sparse -Wsparse-error hello.c > hello.sparse
    build failed: hello.sparse
    Build failed 2/2 failures

This fails and shows us that we ought to have specified a return type
for `main`.  It also possibly reruns `python3 configure.py`.  This is
because the `glob.glob('*.c')` reads the repository directory, and
since the linting modified the repository directory it is possible
that the output of configuring would have changed.

## Using `fac --continual`

Suppose we really like linting while we are editing.  In this case, we
might want to execute in a convenient terminal:

     $ fac --continual sparse.log

This will wait for any changes to be made, and then re-run sparse.
Thus you can edit away and see if you are introducing new problems, or
have fixed existing warnings.  So let's make an edit:

##### hello.c
    #include <stdio.h>
    
    int main() {
      printf("Hello world!\n");
    }

You should now see in your shell that sparse passes.  Yay!

## Using git hooks

Now if you are very lazy (or perhaps want to ensure a test suite is
already run), you can tell git to run fac for you when you do a commit
(or add).

##### .git/hooks/pre-commit
    #!/bin/sh
    set -ev
    fac

Now when you make a commit, fac will automagically build your code,
and if the build fails then the commit will be reversed.  (Note that
this is a bit sloppy: the build may not be what is staged for commit.
That would take more work.)

    $ chmod +x .git/hooks/pre-commit
    $ git commit -am 'fix sparse problem in main'
    fac
    1/1 [...s]: python3 configure.py
    2/3 [...s]: gcc -c hello.c
    3/3 [...s]: gcc -o hello hello.o
    Build succeeded! 0:0...
    ...

The tricky bit here is that you are now running two instances of fac
in the same directory at the same time.  This is okay, since fac takes
a lock in the repository before doing any building, so the two should
not interfere with each other.

That concludes this little tutorial.  Hopefully this will help you to
see one way that you can use fac with both `--continual` and git
hooks, as well as optional rules, to customize your build.
