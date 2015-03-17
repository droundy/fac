#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

mkdir subdir

cat > subdir/foo.fac <<EOF
| echo foo > foo

| echo bar > bar

| cat foo bar > baz
< bar
< foo
EOF

git init
git add subdir/foo.fac

../../fac

grep foo subdir/foo
grep bar subdir/bar
grep foo subdir/baz
grep bar subdir/baz

exit 0
