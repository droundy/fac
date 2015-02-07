# Comparison with SCons

## A story of a bug

I was switching my research code from SCons to fac, and I ran into the
following error:

    4/92 [0.57s]: python2 figs/w2-comparison.py
    Traceback (most recent call last):8      
      File "figs/w2-comparison.py", line 12, in <module>
        import wca_erf
    ImportError: No module named wca_erf

This error surprised me.  We had been compiling this code regularly,
and never ran into trouble.  I looked in the directory, and there was
no sign of any `wca_erf.py` file.  I looked in my student's directory,
and there was a backup file `wca_erf.py~`, and a compiled
`wca_erf.pyc` file, but again no sign of the source code itself.
Finally, I thought to search in our git history:

    $ git log --stat wca_erf.py
    commit d4b7bd085eab4aff554802ff63336c6f407af5ef
    Author: [redacted]
    Date:   Wed Dec 3 11:36:06 2014 -0800
    
        remove unused and wrong code
    
    papers/.../wca_erf.py |   16 ----------------
    1 file changed, 16 deletions(-)

So this module was deleted when a bug was found in it, and we didn't
notice that it was still required in order to run two of our scripts
that generate plots for our paper.  For three months, we didn't
notice.  How did this happen?

The problem was that we had enabled scons caching.  This is a major
feature of scons, that it can cache the results of builds.  This is
very helpful, as a complete build of our code takes close to an hour.
Scons uses checksums to verify whether the inputs are identical to the
previous build, including things you might not think of like
environment variables, so this is supposed to be safe.  However, scons
relies on the user (or clever scripts) to determine the dependencies,
and in this case our scripts failed to recognize this import as
introducing a dependency on the source file, with the result that when
the file was deleted, scons continued to believe--so long as we didn't
modify the scripts that import it--that it knew the result of running
those scripts and didn't have to try running them.  So the build
continued succeeding, and would have continued doing so until some
poor soul tried to build the code in an account that didn't have this
file cached.

I have not yet implemented caching for fac, but when I do, it will be
safe from this sort of error, because fac will *know* the *true*
dependencies of each rule.
