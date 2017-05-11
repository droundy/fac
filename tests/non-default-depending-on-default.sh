#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| echo foo > foo
> foo

? cat foo > bar
> bar
< foo

| cat foo > ugg
> ugg
< foo
EOF

git init
git add top.fac

${FAC:-../../fac}

grep foo foo
grep foo ugg

test ! -e bar

exit 0
