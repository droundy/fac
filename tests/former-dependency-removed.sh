#!/bin/sh

set -ev

echo $0

# This test ensures that we can count builds properly to figure out if
# we have built everything correctly.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| sh script.sh > foo
> foo

EOF

cat > script.sh <<EOF
#!/usr/bin/sh

set -e

cat input

EOF

echo hello > input

git init
git add top.fac input script.sh

${FAC:-../../fac}

grep hello foo

sleep 1
echo goodbye > input

${FAC:-../../fac}

grep goodbye foo

cat > script.sh <<EOF
#!/usr/bin/sh

set -e

echo hello

EOF

rm input

${FAC:-../../fac}

grep hello foo

exit 0
