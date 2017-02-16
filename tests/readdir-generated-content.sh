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

../../fac -v

grep hello foo/hello
grep world foo/world

# foo-listing may not be correct the first time, since content is generated in foo
cat foo-listing

sleep 1

../../fac -v

grep hello foo/hello
grep world foo/world

cat foo-listing

grep hello foo-listing
grep world foo-listing

cat >> top.fac <<EOF
| echo wonderful > foo/wonderful
EOF

sleep 1

../../fac -v

# have to run twice because first time we modify the directory.  Should we make this build in just one rune?

../../fac -v

grep wonderful foo/wonderful

# The foo-listing should be updated, since we have added a new file to
# the foo directory.
cat foo-listing

grep wonderful foo-listing
grep hello foo-listing
grep world foo-listing

exit 0
