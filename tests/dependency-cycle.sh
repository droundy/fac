#!/bin/sh

set -ev

echo $0

# This test ensures that we can count builds properly to figure out if
# we have built everything correctly.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| cat bar > foo
> foo
< bar

| cat foo > baz
> baz
< foo

| cat baz > bar
< baz
> bar

EOF

git init
git add top.fac

if ../../fac foo; then
  echo this should fail as a cycle
  exit 1
fi

if ../../fac; then
  echo this should fail as a cycle
  exit 1
fi

exit 0
