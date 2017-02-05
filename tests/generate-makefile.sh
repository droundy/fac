#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > build.fac <<EOF
| echo awesome > awesome
> awesome

| echo foo > foo

# The following is malformed output

| echo hello > /tmp/mistake && echo baz > baz
> /tmp/mistake

EOF

git init
git add build.fac

if ../../fac --makefile Makefile; then
    echo this should have failed, since it has malformed output
fi

cat > build.fac <<EOF
| echo awesome > awesome
> awesome

| echo foo > foo

| echo hello > /tmp/mistake && echo baz > baz

EOF

../../fac --makefile Makefile

grep awesome awesome
grep foo foo
grep baz baz

cat Makefile

../../fac -c

make

grep awesome awesome
grep foo foo
grep baz baz

exit 0
