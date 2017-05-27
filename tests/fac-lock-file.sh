#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > my.fac <<EOF
| sleep 15 && echo good > good
EOF

git init
git add my.fac

${FAC:-../../fac} &

sleep 1

if ${FAC:-../../fac} &> output; then
    echo should have failed due to lock
    cat output
    exit 1
fi

cat output
grep git/fac-lock output

exit 0
