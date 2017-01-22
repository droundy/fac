#!/bin/sh

# This test is for issue #9 on github.

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| mkdir foo

| echo welcome > foo/bar
< foo
EOF

git init
git add top.fac

../../fac

grep welcome foo/bar

test -d foo

cat top.fac.tum

../../fac -c

if grep welcome foo/bar; then
  echo foo/bar should have been deleted
  exit 1
fi

if test -d foo; then
  echo directory foo should have been deleted
  exit 1
fi

exit 0
