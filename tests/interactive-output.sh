#!/bin/sh

set -ev

if ! which screen; then
    echo there is no screen available
    exit 137
fi

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > my.fac <<EOF
| sleep 1 && echo good > good

| sleep 1 && echo good > better

| sleep 1 && echo good > best

EOF

git init
git add my.fac

screen -m -D -L output ../../fac --jobs=1

cat output
grep 'Build time remaining' output

exit 0
