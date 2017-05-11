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
< foo
EOF

git init
git add foo.fac

${FAC:-../../fac}

grep foo foo
grep bar bar
grep foo baz
grep bar baz

# This test ensures that we can build even when we have rules that do
# not define their output files, and the files already exist.

cat > foo.fac <<EOF
| echo foo > foo

| echo bar > bar

| cat foo bar hello > baz
< bar
EOF

echo hello > hello
git add hello

${FAC:-../../fac}

grep foo foo
grep bar bar
grep foo baz
grep bar baz
grep hello baz

exit 0
