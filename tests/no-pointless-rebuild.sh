#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.fac <<EOF
| echo foo > foo
> foo
EOF

git init
git add top.fac

${FAC:-../../fac} -v > fac.out
cat fac.out
if grep 'Building foo' fac.out; then
    echo It is dirty, as it should be.
else
    echo It should not be clean!!!
    exit 1
fi

cat top.fac.tum

${FAC:-../../fac} -v > fac.out
cat fac.out
if grep 'Building foo' fac.out; then
    echo It is dirty, but should not be!!!
    exit 1
else
    echo All is well.
fi
if grep '1/1' fac.out; then
    echo It is dirty, but should not be!!!
    exit 1
else
    echo All is well.
fi

exit 0
