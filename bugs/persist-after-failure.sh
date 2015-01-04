#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir
cat > top.bilge <<EOF
| echo foo > foo
> foo

| cat foo bar > foobar
> foobar

| echo bar > bar
> bar

EOF

../../bilge || true

grep foo foo
grep bar bar

../../bilge

grep foo foo
grep bar bar
grep foo foobar
grep bar foobar

exit 0
