#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.bilge <<EOF
| echo *.message > messages
> messages
EOF

touch foo.message

git init
git add top.bilge foo.message

../../bilge

ls -lhd .

sleep 2

grep foo.message messages

touch bar.message
../../bilge

ls -lhd .

grep foo.message messages
grep bar.message messages

exit 0
