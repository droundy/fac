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
      int d;
      printf(
      "Hello world!\n"
         );
    }

##### .clang-tidy
    Checks: '*'
    WarningsAsErrors: '*'
    HeaderFilterRegex: '.*'

We will create a git repository and add this file to it.

    $ git init
    Initialized empty Git repository in ...
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
    for c in glob.glob('*.c'):
        cmd = 'clang-3.9 -c %s' % c
        objects.append(c[:-1]+'o')
        clang_database.write('''{"directory": "%s",
                                 "command": "%s",
                                 "file": "%s"}
                             ''' % (os.getcwd(), cmd, c))
        facfile.write('| %s\n> %s\n' % (cmd, objects[-1]))
    clang_database.write(']\n')
    facfile.write('| clang-3.9 -o hello ' + ' '.join(objects) + '\n> hello\n')
    for o in objects:
        facfile.write('< %s\n' % o)

This program also generates a clang compile database, which is helpful
for the clang tooling.  In our example, we will be using clang-tidy to
lint our code.

##### configure.fac
    | python3 configure.py
    > .fac
    
    ? clang-tidy-3.9 -warnings-as-errors=* *.c > lint-output || (cat lint-output && false)
    > lint-output

You might wonder, why all the trickiness with the `clang-tidy` rule?
The optional aspect `?` is because I don't expect everyone compiling
the program to have `clang-tidy`, but want them to not have to type
`fac hello` to get a successful build.  The redirection of output and
shell trickery to write it also to `stdout` accomplishes two things.
First, it ensures that this rule creates a file, which enables fac to
avoid rerunning the lint when nothing has changed.  Secondly, it
ensures that when the command fails we can see the output (on stdout).

Now we can just build the executable without running the by calling
`fac` with no arguments.

    $ git add configure.fac configure.py
    $ git commit -am 'first version'
    ...
    $ fac
    1/1 [...s]: python3 configure.py
    2/3 [...s]: clang-3.9 -c hello.c
    3/3 [...s]: clang-3.9 -o hello hello.o
    ...
    Build succeeded! ...

Or we can just run the lint (clang-tidy) by

    $ fac lint-output # fails
    1 warning generated.
    1 warning treated as error
    ... error: type specifier missing, ...
    main() {
    ^
    ...

This fails and shows us that we ought to have specified a return type
for `main`.  It also possibly reruns `python3 configure.py`.  This is
because the `glob.glob('*.c')` reads the repository directory, and
since the linting modified the repository directory it is possible
that the output of configuring would have changed.

## Using `fac --continual`

Suppose we really like linting while we are editing.  In this case, we
might want to execute in a convenient terminal:

     $ fac --continual lint-output

This will wait for any changes to be made, and then re-run
clang-tidy.  Thus you can edit away and see if you are introducing new
problems, or have fixed existing warnings.  So let's make an edit:

##### hello.c
    #include <stdio.h>
    
    int main() {
      int d;
      printf(
      "Hello world!\n"
         );
    }

You should now see in your shell that clang-tidy passes.  Yay!

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
    $ git commit -am 'fix lint problem in main'
    fac
    1/1 [...s]: python3 configure.py
    2/3 [...s]: clang-3.9 -c hello.c
    3/3 [...s]: clang-3.9 -o hello hello.o
    Build succeeded! 0:0...
    ...

The tricky bit here is that you are now running two instances of fac
in the same directory at the same time.  This is okay, since fac takes
a lock in the repository before doing any building, so the two should
not interfere with each other.
