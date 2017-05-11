#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| cp input.o output
EOF

cat > .gitignore <<EOF
*.o
EOF

echo input > input.o

git init
git add top.fac .gitignore

if ${FAC:-../../fac} --git-add; then
    echo this should have failed due to not adding input.o
    exit 1
fi

git add -f input.o

${FAC:-../../fac}

exit 0
