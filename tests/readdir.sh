#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.loon <<EOF
| echo *.message > messages
> messages
EOF

touch foo.message

git init
git add top.loon foo.message

../../loon

ls -lhd .

sleep 2

grep foo.message messages

touch bar.message
../../loon

ls -lhd .

grep foo.message messages
grep bar.message messages

exit 0
