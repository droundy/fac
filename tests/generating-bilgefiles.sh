#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

git init

cat > top.fac <<EOF
| python fac.py > 1.fac
> 1.fac
EOF
git add top.fac

cat > fac.py <<EOF
print """
| python fac2.py > 2.fac
> 2.fac
"""
EOF
git add fac.py

cat > fac2.py <<EOF
print """
| echo foo > foo
> foo
"""
EOF
git add fac2.py

git ls-files

../../fac -v

grep foo foo

../../fac --clean

if test -e foo; then
  echo file foo should have been deleted
  exit 1
fi

../../fac

grep foo foo

exit 0
