#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > build.fac <<EOF
| echo awesome > awesome
> awesome

| echo foo > foo

# The following is malformed, but does not trigger bug.

| echo hello > /tmp/mistake && echo baz > baz
> /tmp/mistake

EOF

git init
git add build.fac

../../fac --makefile Makefile

grep awesome awesome
grep foo foo
grep baz baz

cat Makefile

fac -c

make

grep awesome awesome
grep foo foo


exit 0
