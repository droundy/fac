#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

git init

cat > top.fac <<EOF
| python fac.py > generated.fac
> generated.fac
EOF
git add top.fac

cat > fac.py <<EOF
print """
| echo this is bad > bad-case
> bad-case
"""

exit(1) # command fails, so this facfile should not be used
EOF
git add fac.py

git ls-files

if ../../fac -v; then
    echo fac should fail, because command fails
    exit 1
fi

if grep bad bad-case; then
    echo the bad command should not have run
    exit 1
fi

../../fac -v > log 2>&1 || echo it failed as expected

cat log

if grep bad log; then
    echo nothing "bad" should have been seen
    exit 1
fi

exit 0
