#!/bin/sh

set -ev

echo $0

# This test ensures that we can count builds properly to figure out if
# we have built everything correctly.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

mkdir subdir

cat > top.fac <<EOF
| cat foo > bar
< foo

EOF

git init
git add top.fac

echo hello > foo

if ${FAC:-../../fac} > fac.out; then
    cat fac.out
    echo this should have failed
    exit 1
fi
cat fac.out

grep 'add .*foo.* to git' fac.out

cat >> top.fac <<EOF
| echo hello > foo

EOF

${FAC:-../../fac}

grep hello bar

exit 0
