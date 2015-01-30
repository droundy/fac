#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

git init

cat > top.loon <<EOF
| python loon.py > 1.loon
> 1.loon
EOF
git add top.loon

cat > loon.py <<EOF
print """
| python loon2.py > 2.loon
> 2.loon
"""
EOF
git add loon.py

cat > loon2.py <<EOF
print """
| echo foo > foo
> foo
"""
EOF
git add loon2.py

git ls-files

../../loon -v

grep foo foo

../../loon --clean

if test -e foo; then
  echo file foo should have been deleted
  exit 1
fi

../../loon

grep foo foo

exit 0
