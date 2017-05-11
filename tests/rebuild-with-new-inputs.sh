#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > my.fac <<EOF
| cat foo bar > out || true

EOF

echo foo > foo

git init
git add my.fac foo

${FAC:-../../fac}

grep foo out
if grep bar out; then
  echo bar should not be there
  exit 1
fi

echo bar > bar
git add bar

${FAC:-../../fac}

# we should not have just rebuilt it, because fac does not know that
# bar could be an input, since we don't track attempts to open
# nonexistent files

grep foo out
if grep bar out; then
  echo bar should not be there
  exit 1
fi

cat > my.fac <<EOF
| cat foo bar > out || true
< bar
EOF

${FAC:-../../fac} --parse-only my.fac

if ${FAC:-../../fac} --parse-only=myfac &> fac.out; then
    echo should have failed because myfac does not exist
    cat fac.out
    exit 1
fi

cat fac.out
grep 'unable to open file' fac.out
grep myfac fac.out

${FAC:-../../fac}

grep foo out
grep bar out

exit 0
