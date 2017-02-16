#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| mkdir foo
EOF

git init
git add top.fac

../../fac

sleep 1

touch foo/bar

# We shouldn't need to rebuild foo just because the directory contents
# have been changed.  Note that we could "fix" this by using mkdir -p,
# but that would just obscure the bug, which is that foo shouldn't be
# rebuilt at all.

../../fac -v

exit 0
