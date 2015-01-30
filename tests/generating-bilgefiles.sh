#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

git init

cat > top.bilge <<EOF
| python bilge.py > 1.bilge
> 1.bilge
EOF
git add top.bilge

cat > bilge.py <<EOF
print """
| python bilge2.py > 2.bilge
> 2.bilge
"""
EOF
git add bilge.py

cat > bilge2.py <<EOF
print """
| echo foo > foo
> foo
"""
EOF
git add bilge2.py

git ls-files

../../bilge -v

grep foo foo

../../bilge --clean

if test -e foo; then
  echo file foo should have been deleted
  exit 1
fi

../../bilge

grep foo foo

exit 0
