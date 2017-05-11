#!/bin/sh

set -ev

echo $0

# This test ensures that we can count builds properly to figure out if
# we have built everything correctly.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| echo foo > foo

| echo bar > bar

| echo zoo > zoo

| cat foo bar zoo > all
< foo
< bar
< zoo

EOF

git init
git add top.fac

${FAC:-../../fac} -v

grep foo foo
grep bar bar
grep zoo zoo

grep foo all
grep bar all
grep zoo all

exit 0
