#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| ls foo > foo-listing
< foo

| mkdir -p foo

| echo hello > foo/hello
< foo

| echo world > foo/world
< foo
EOF

git init
git add top.fac

${FAC:-../../fac} -vvv

grep hello foo/hello
grep world foo/world

# foo-listing may not be correct the first time, since content is generated in foo
cat foo-listing
cp foo-listing old-foo-listing

sleep 1

${FAC:-../../fac} -vvv

grep hello foo/hello
grep world foo/world

cat foo-listing
diff foo-listing old-foo-listing

rm foo-listing

${FAC:-../../fac} -vvv

grep hello foo-listing
grep world foo-listing

exit 0
