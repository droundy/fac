#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

# This test will fail if we are unable to identify paths within .git/
# properly.

cat > top.fac <<EOF
| git describe > version
EOF

git init
git add top.fac
git commit -am 'first version'
git tag -a 0.0 -m 'The first version'

${FAC:-../../fac}

cat version
grep 0.0 version

exit 0

