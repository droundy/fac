#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.loon <<EOF
| echo foo > foo
> foo

| cat foo > baz
> baz
< foo
EOF

git init
git add top.loon

../../loon

grep foo baz
grep foo foo

../../loon --clean

if test -e foo; then
  echo file foo should have been deleted
  exit 1
fi

if test -e baz; then
  echo file foo should have been deleted
  exit 1
fi

exit 0
