#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir
cat > top.bilge <<EOF
| echo foo > foo
> foo

| echo bar > bar
> bar

| cat foo bar > foobar
> foobar
< foo
< bar
EOF

../../bilge

grep foo foo
grep bar bar
grep foo foobar
grep bar foobar

exit 0
