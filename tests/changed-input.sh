#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| sh script.sh > foo
> foo
EOF

cat > script.sh <<EOF
cat input
EOF

echo foo > input

git init
git add top.fac input script.sh

../../fac

grep foo foo

grep input top.fac.tum

echo bars > input

../../fac

grep bar foo

cat > script.sh <<EOF
cat input2
EOF

git mv input input2
echo baz > input2

../../fac

grep baz foo


exit 0
