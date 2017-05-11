#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

# This script tests the case where we have a command that generates a
# given output *unless it already exists*.  That command then is
# removed from the fac file, which means that fac forgets that it has
# generated that output.  Then the rule is added back.  If the output
# still exists, then fac will think that it is *input*, not *output*
# and will complain that it needs to be added to git.  So we want to
# clean the output of commands that have been removed or changed.

cat > top.fac <<EOF
| python2 rules.py > generated.fac
> generated.fac
c .pyc

EOF

cat > rules.py <<EOF
import os

if os.getenv("NOFOO") == None:
    print '| if ! test -e foo; then echo foo > foo; fi'
else:
    print '# No foo rule', os.getenv("NOFOO")

print '| echo bar > bar'
EOF

git init
git add top.fac rules.py

${FAC:-../../fac}

cat generated.fac

grep 'echo foo' generated.fac
grep 'echo foo' generated.fac.tum
grep foo foo

${FAC:-../../fac}

grep 'echo foo' generated.fac
grep 'echo foo' generated.fac.tum
grep foo foo

NOFOO=true ${FAC:-../../fac}

cat generated.fac

if grep 'echo foo' generated.fac; then
    echo generated.fac is wrong
    exit 1
fi
grep 'No foo rule' generated.fac
test ! -e foo # The output file foo should have been deleted

${FAC:-../../fac}

grep 'echo foo' generated.fac
grep 'echo foo' generated.fac.tum
grep foo foo

exit 0
