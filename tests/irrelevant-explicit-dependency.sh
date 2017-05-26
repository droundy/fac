#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

git init

cat > top.fac <<EOF
| echo hello > greeting
< fake-input
EOF

git add top.fac

if ${FAC:-../../fac}; then
    echo should have failed
    exit 1
fi

echo diversion > fake-input

if ${FAC:-../../fac}; then
    echo should have failed
    exit 1
fi

git add fake-input

${FAC:-../../fac}

grep hello greeting

${FAC:-../../fac} > fac.out
cat fac.out

if grep echo fac.out; then
    echo we rebuilt when we should not have
    exit 1
fi

exit 0
