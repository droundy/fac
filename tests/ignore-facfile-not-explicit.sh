#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| echo "| echo foo > foo" > generated.fac
EOF

git init
git add top.fac

../../fac

grep foo generated.fac

if grep foo foo; then
    echo there should be no foo
    exit 1
fi

cat > top.fac <<EOF
| echo "| echo foo > foo" > generated.fac
> generated.fac
EOF

../../fac

# now the generated rules should be run!
grep foo foo

exit 0
