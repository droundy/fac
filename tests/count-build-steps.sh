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
| cat subdir/foo subdir/bar > foobar
> foobar
< subdir/foo
< subdir/bar

EOF

cat > subdir/.fac <<EOF
| echo foo > foo
> foo

| echo bar > bar
> bar

EOF

git init
git add top.fac subdir/.fac

${FAC:-../../fac} > fac.out
cat fac.out

grep foo foobar
grep bar foobar

grep 1/3 fac.out
grep 2/3 fac.out
grep 3/3 fac.out | grep foobar

exit 0
