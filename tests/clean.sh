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

| cat input > output
< input
> output
EOF

echo goodness > input

git init
git add top.fac input

../../fac

grep foo baz
grep foo foo
grep goodness output

../../fac --clean -v

if test -e foo; then
  echo file foo should have been deleted
  exit 1
fi

if test -e baz; then
  echo file foo should have been deleted
  exit 1
fi

grep goodness input

if test -e output; then
  echo file output should have been deleted
  exit 1
fi

exit 0
