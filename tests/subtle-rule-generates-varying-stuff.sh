#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| sh subtle-rule.sh

EOF

cat > subtle-rule.sh <<EOF

if ! test -e foo; then
  echo foo > foo
fi

EOF

git init
git add top.fac subtle-rule.sh

../../fac

grep foo foo

grep foo top.fac.tum | grep '>'

cat > subtle-rule.sh <<EOF

if ! test -e foo; then
  echo foo > foo
fi

if ! test -e bar; then
  echo bar > bar
fi

EOF

../../fac

grep foo top.fac.tum | grep '>'
grep bar top.fac.tum | grep '>'

grep foo foo
grep bar bar

# Here is a build rule that sadly is not idempotent:

cat > subtle-rule.sh <<EOF

if ! test -e foo; then
  echo wrong > foo
fi

if ! test -e bar; then
  echo bar > bar
fi

EOF

../../fac

grep foo top.fac.tum | grep '>'
grep bar top.fac.tum | grep '>'

grep foo foo
grep bar bar

rm foo

../../fac

grep foo top.fac.tum | grep '>'
grep bar top.fac.tum | grep '>'

grep wrong foo
grep bar bar

# Now the subtle-rule.sh no longer will generate foo.

cat > subtle-rule.sh <<EOF

if ! test -e bar; then
  echo bar > bar
fi

EOF

../../fac --show-output

# Fac continues to believe that the rule will generate foo, so long as
# foo still exists.

grep foo top.fac.tum | grep '>'
grep bar top.fac.tum | grep '>'

grep wrong foo
grep bar bar

rm foo

../../fac --show-output

# Once foo is deleted and the command does not regenerate it, fac
# should remove it from the output list.

if grep foo top.fac.tum | grep '>'; then
    echo Oh no this is bad
    exit 1
fi
grep bar top.fac.tum | grep '>'

grep bar bar

exit 0
