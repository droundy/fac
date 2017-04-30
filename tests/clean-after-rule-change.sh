#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| echo foo > foo

| echo bar > baz
EOF

git init
git add top.fac

../../fac

ls -lh

grep foo foo
grep bar baz

echo Oh no, there was a bug in our rule!

cat > top.fac <<EOF
| echo foo > foo

| echo bar > bar
EOF

../../fac

ls -lh

grep foo foo
grep bar bar

../../fac --clean -v

if test -e foo; then
  echo file foo should have been deleted
  exit 1
fi

if test -e bar; then
  echo file bar should have been deleted
  exit 1
fi

if test -e baz; then
  echo file baz should have been deleted
  exit 1
fi

ls -lh

exit 0
