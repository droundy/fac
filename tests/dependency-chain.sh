#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir
cat > top.bilge <<EOF
| echo foo > foo
> foo

| cat foo > bar
> bar
< foo

| cat bar > baz
> baz
< bar
EOF

../../bilge

grep foo foo
grep foo bar
grep foo baz

exit 0