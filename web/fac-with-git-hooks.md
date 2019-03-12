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
    
    int main() {
      printf("Hello dang world!\n");
    }

We will use a very simple lint program that ensures that inappropriate
words do not work their way into our source code.

##### check-for-curses.py
    import sys
    retval = 0
    with open(sys.argv[1]) as f:
      contents = f.read()
      for profanity in ['dang', 'shucks']:
          if contents.find(profanity) >= 0:
              print('Watch your language: "{}" in file'.format(profanity), sys.argv[1])
              retval += 1
    if retval == 0:
        print('File', sys.argv[1], 'contains no inappropriate language!')
    else:
        print('File', sys.argv[1], 'has', retval, 'problems!')
    exit(retval)

We will create a git repository and add these files to it.

    $ git init
    Initialized empty Git repository in ...
    $ git config user.email "testing@example.com"
    $ git config user.name "Tester"
    $ git add hello.c check-for-curses.py

Now let's get started with figuring out how to build this.  Although
we have only one C file, we anticipate creating a large program, so we
don't want to list all the C files manually.  Instead we will write a
program that generates a facfile called `.fac` that explains how to
compile everything.  For a real program, I would put in more
intelligence here to check what compilers are available, and what
flags are appropriate for said compilers, but let's keep this simple.

##### configure.py
    import glob, os
    facfile = open('.fac', 'w')
    objects = []
    curses = []
    for c in glob.glob('*.c'):
        objects.append(c[:-1]+'o')
        curses.append(c[:-1]+'curse')
        facfile.write('| gcc -c %s\n' % c)
        facfile.write('? python3 check-for-curses.py %s > %s\n> %s\n'
                      % (c, curses[-1], curses[-1]))
    facfile.write('| gcc -o hello ' + ' '.join(objects) + '\n> hello\n')
    for o in objects:
        facfile.write('< %s\n' % o)
    facfile.write('? cat %s > curses.log\n> curses.log\n' % ' '.join(curses))
    for o in curses:
        facfile.write('< %s\n' % o)

We create a file `configure.fac` which just tells fac to run
`configure.py` to find out what to do.

##### configure.fac
    * python3 configure.py
    > .fac

We use a `*` rule here because configure uses globs ("%.c") and
therefore may need to be rerun if directory contents change.

You might wonder, why all the trickiness with the
`check-for-curses.py` rules?  The optional aspect `?` is because I
don't expect everyone compiling the program to have the tools needed
for checking the source code (e.g. sparse).  So I don't want them to
have to type `fac hello` to get a successful build.  The `cat` of many
(in our case one) `*.curse` files into `curses.log` is so we can have a
single fac target that checks the status of any source files that need
to be checked (but no more).  I'm assuming this may be a static
checker that is slow, or even a little fuzzer.

Now we can just build the executable without running the by calling
`fac` with no arguments.

    $ git add configure.fac configure.py
    $ git commit -am 'first version'
    ...
    $ fac
    1/... [...s]: python3 configure.py
    2/3 [...s]: gcc -c hello.c
    3/3 [...s]: gcc -o hello hello.o
    ...
    Build succeeded! ...

Or we can just run the lint by

    $ fac
    ...
    $ # I am first running it manually to show you the output
    $ python3 check-for-curses.py hello.c # fails
    Watch your language: "dang" in file hello.c
    File hello.c has 1 problems!
    $ fac curses.log # fails
    ... build failed: python3 check-for-curses.py hello.c > hello.curse
    Build failed 2/2 failures ...

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
    fac curses.log

Now when you make a commit, fac will automagically build your code,
and if the build fails then the commit will be reversed.  (Note that
this is a bit sloppy: the build may not be what is staged for commit.
That would take more work.)

    $ chmod +x .git/hooks/pre-commit
    $ git commit -am 'fix profanity problem in main'
    fac curses.log
    1/2 [...s]: python3 check-for-curses.py hello.c > hello.curse
    2/2 [...s]: cat hello.curse > curses.log
    Build succeeded! ...
    ...

The tricky bit here is that you are now running two instances of fac
in the same directory at the same time.  This is okay, since fac takes
a lock in the repository before doing any building, so the two should
not interfere with each other.

To make clear that the test works, we can now introduce a new error:

##### hello.c
    #include <stdio.h>
    
    int main() {
      printf("Hello world shucks!\n");
    }

    $ ! git commit -am 'break the build'
    fac curses.log
    1/1 [...s]: python3 configure.py
    !2/3! [...s]: build failed: python3 check-for-curses.py hello.c > hello.curse
    Build failed ...
    ...

That concludes this little tutorial.  Hopefully this will help you to
see one way that you can use fac with both `--continual` and git
hooks, as well as optional rules, to customize your build.

