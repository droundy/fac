#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| echo "| echo foo > foo" > foo.fac
> foo.fac
C ~/.cache
c .pyc

| cat foo > baz
C ~/.cache
c .pyc
> baz
< foo

EOF

echo goodness > input

git init
git add top.fac input

env - ../../fac

exit 0
