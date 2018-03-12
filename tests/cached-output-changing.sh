#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| sh generate-stuff.sh

EOF

cat > generate-stuff.sh <<EOF
set -ev

test -e foo || echo foo > foo

test -e bar || echo bar > bar

EOF

git init
git add top.fac generate-stuff.sh

echo hello > foo

if ${FAC:-../../fac}; then
    echo This should fail because foo exists
fi

ls -l
grep hello foo
echo bar should ideally not exist, because command failed

rm -f foo bar

${FAC:-../../fac}

grep foo foo
grep bar bar


cat > generate-stuff.sh <<EOF
set -ev

test -e foo || echo foo > foo
test -e bar || echo bar > bar
test -e baz || echo baz > baz

EOF

${FAC:-../../fac}

grep foo foo
grep bar bar
grep baz baz

echo badness > baz

cat top.fac.tum

${FAC:-../../fac}

grep foo foo
grep bar bar
grep badness baz

cat top.fac.tum

rm baz

${FAC:-../../fac}

grep foo foo
grep bar bar
grep baz baz

exit 0
