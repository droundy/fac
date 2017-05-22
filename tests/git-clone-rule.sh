#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

git init

cat > git-cloning.fac <<EOF
| if cd bigbro; then git pull; else git clone --depth 3 git://github.com/droundy/bigbro.git bigbro; fi
> bigbro/configure.fac
C bigbro/.git/ref/
EOF
git add git-cloning.fac

export RUST_BACKTRACE=1

${FAC:-../../fac}

grep bigbro/configure.py git-cloning.fac.tum

echo bad-news >> bigbro/README.md

${FAC:-../../fac}

grep bigbro/configure.py git-cloning.fac.tum

cd bigbro
git reset --hard HEAD~1
cd ..

${FAC:-../../fac}

grep bigbro/configure.py git-cloning.fac.tum

exit 0
