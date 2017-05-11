#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo hello > input_file

mkdir repo
cd repo

cat > top.fac <<EOF
| sh script.sh > foobar
< script.sh
> foobar
EOF

cat > script.sh <<EOF
cat ../input_file
EOF

git init
git add top.fac script.sh

${FAC:-../../../fac}

grep hello foobar


mv ../input_file ../new_input_file
echo goodbye > ../new_input_file

if ${FAC:-../../../fac}; then
  echo this should have failed
  exit 1
fi

cat > script.sh <<EOF
cat ../new_input_file
EOF

${FAC:-../../../fac}

grep goodbye foobar

exit 0
