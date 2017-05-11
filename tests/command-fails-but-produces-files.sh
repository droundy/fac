#!/bin/sh

set -ev

echo $0

# This test ensures that we can count builds properly to figure out if
# we have built everything correctly.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| sh cmd.sh

EOF

cat > cmd.sh <<EOF
if test ! -e hello; then echo hello > hello; fi
exit 1
EOF

git init
git add top.fac cmd.sh

if ${FAC:-../../fac} --log-ouput log > fac.out; then
    cat fac.out
    echo this should have failed
    exit 1
fi
cat fac.out

if grep hello hello; then
    echo should not have created hello
    exit 1
fi

cat > cmd.sh <<EOF
if test ! -e hello; then echo hello > hello; fi
exit 0
EOF

${FAC:-../../fac}

grep hello hello

exit 0
