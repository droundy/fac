#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

# 1. Fix error message when a file is needed to build another file that
#    is needed for a third one, where there are optional rules
#    involved.  We currently use pretty_rule, but ought to show the
#    actual file dependency that leads to the build.

cat > my.fac <<EOF
? cat f1 > f2 && cat f1 > g1 && cat f1 > g2
> g1
> f2
> g2
< f1

? cat f1 > h2 && cat f1 > j1 && gat f1 > j2
> j1
> h2
> j2
< f1

| cat f2 > f3
> f3
< f2
EOF

git init
git add my.fac

if ${FAC:-../../fac} > out; then
  cat out
  echo build should fail missing file f1
  exit 1
fi

cat out

# We don't need g1 or g2, we need f2, so that should be mentioned in
# the error message.
grep f2 out

if grep g1 out; then
  echo we should not talk about g1
  exit 1
fi

if grep g2 out; then
  echo we should not talk about g2
  exit 1
fi

echo hello > f1

if ${FAC:-../../fac} > out; then
  echo build should fail file f1 is not in git
  exit 1
fi

cat out

# We don't need g1 or g2, we need f2, so that should be mentioned in
# the error message.
grep f2 out

if grep g1 out; then
  echo we should not talk about g1
  exit 1
fi

if grep g2 out; then
  echo we should not talk about g2
  exit 1
fi

git add f1

${FAC:-../../fac} -v

exit 0
