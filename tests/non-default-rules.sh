#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF

| cat foo bar baz > output
< foo
< bar
< baz

EOF

for x in foo bar baz hello stupid bad horrible; do
  cat >> top.fac <<EOF
? echo $x > $x
> $x

EOF
done

git init
git add top.fac

../../fac

grep foo foo
grep bar bar
grep baz baz

grep foo output
grep bar output
grep baz output

test ! -e hello
test ! -e stupid
test ! -e bad
test ! -e horrible

../../fac hello

grep hello hello
test ! -e stupid
test ! -e bad
test ! -e horrible

exit 0
