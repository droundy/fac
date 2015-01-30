#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| echo foo > foo
> foo

| cat foo > baz
> baz
< foo
EOF

git init
git add top.fac

../../fac

grep foo baz
grep foo foo

../../fac --clean

if test -e foo; then
  echo file foo should have been deleted
  exit 1
fi

if test -e baz; then
  echo file foo should have been deleted
  exit 1
fi

exit 0
