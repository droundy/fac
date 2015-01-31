#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > foo.fac <<EOF
| echo foo > foo
> foo

| echo bar > bar
> bar

| cat foo bar > baz
< bar
EOF

git init
git add foo.fac

../../fac

grep foo foo
grep bar bar
grep foo baz
grep bar baz

cat > foo.fac <<EOF
| echo foo > foo

| echo bar > bar

| cat foo bar hello > baz
< bar
EOF

echo hello > hello
git add hello

../../fac

grep foo foo
grep bar bar
grep foo baz
grep bar baz
grep hello baz

exit 0
