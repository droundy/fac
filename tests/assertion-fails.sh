#!/bin/sh

set -ev

echo $0

# This test ensures that we can count builds properly to figure out if
# we have built everything correctly.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| ../assertion-fails.test > foo 2>&1
> foo

EOF

git init
git add top.fac

if ${FAC:-../../fac} --log-ouput log > fac.out; then
    cat fac.out
    echo this should have failed
    exit 1
fi
cat fac.out

exit 0
