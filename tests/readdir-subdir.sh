#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| ls subdir > contents
EOF

git init
git add top.fac

mkdir subdir
touch subdir/hello
git add subdir/hello

../../fac

cat top.fac.tum

grep hello contents

sleep 2

touch subdir/goodbye

../../fac

ls -lhd .

grep hello contents
grep goodbye contents

exit 0
