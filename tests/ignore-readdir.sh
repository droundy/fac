#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| echo *.message > messages
> messages
EOF

touch foo.message

git init
git add top.fac foo.message

${FAC:-../../fac}

ls -lhd .

sleep 2

grep foo.message messages

touch bar.message
${FAC:-../../fac}

ls -lhd .

grep foo.message messages
if grep bar.message messages; then
    echo should not have rerun for '|' rule
    exit 1
fi

exit 0
