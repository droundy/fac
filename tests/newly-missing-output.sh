#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > foo.sh <<EOF
echo foo > foo
echo bar > bar
EOF

cat > my.fac <<EOF
| sh foo.sh
> foo
EOF

git init
git add my.fac foo.sh

../../fac

grep foo foo
grep bar bar
grep foo my.fac.tum
grep bar my.fac.tum

cat > foo.sh <<EOF
echo good > foo
EOF

../../fac

grep good foo
grep bar bar
grep foo my.fac.tum
grep bar my.fac.tum

rm bar
echo foo.sh should be rerun to try to generate bar

../../fac -v > fac.out
cat fac.out

grep 'bar has wrong' fac.out

grep good foo
grep foo my.fac.tum

if grep bar my.fac.tum; then
    echo bar is not relevant
    exit 1
fi

exit 0
