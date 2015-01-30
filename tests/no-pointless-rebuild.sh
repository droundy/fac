#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.loon <<EOF
| echo foo > foo
> foo
EOF

git init
git add top.loon

../../loon -v > loon.out
cat loon.out
if grep dirty loon.out; then
    echo It is dirty, as it should be.
else
    echo It should not be clean!!!
    exit 1
fi

cat top.loon.done

../../loon -v > loon.out
cat loon.out
if grep dirty loon.out; then
    echo It is dirty, but should not be!!!
    exit 1
else
    echo All is well.
fi

exit 0
