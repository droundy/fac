#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > my.fac <<EOF
| echo foo bar > foo.fac
EOF

git init
git add my.fac

${FAC:-../../fac}

cat > my.fac <<EOF
| echo foo bar > foo.fac
> foo.fac
EOF

if ${FAC:-../../fac}; then
    echo should have crashed parsing foo.fac
    exit 1
fi


exit 0
