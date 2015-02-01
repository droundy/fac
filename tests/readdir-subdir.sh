#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| ls subdir > contents
EOF

mkdir subdir

git init
git add top.fac

if ../../fac; then
  echo we should fail due to subdir not being in git
  exit 1
fi

touch subdir/hello
git add subdir/hello

cat top.fac.tum

../../fac

grep hello contents

sleep 2

touch subdir/goodbye

../../fac

ls -lhd .

grep hello contents
grep goodbye contents

exit 0
