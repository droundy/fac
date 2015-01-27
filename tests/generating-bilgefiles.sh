#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.bilge <<EOF
| python bilge.py > 1.bilge
> 1.bilge
EOF

cat > bilge.py <<EOF
print """
| python bilge2.py > 2.bilge
> 2.bilge
"""
EOF

cat > bilge2.py <<EOF
print """
| echo foo > foo
> foo
"""
EOF

../../bilge

grep foo foo

../../bilge --clean

if test -e foo; then
  echo file foo should have been deleted
  exit 1
fi

../../bilge

grep foo foo

exit 0
