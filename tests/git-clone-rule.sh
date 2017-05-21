#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

# expect rust failure

git init

cat > git-cloning.fac <<EOF
| if cd fac-testing; then git pull; else git clone --depth 3 git://github.com/droundy/bigbro.git fac-testing; fi
> fac-testing/configure.fac
C fac-testing/.git/ref/
EOF
git add git-cloning.fac

${FAC:-../../fac}

grep fac-testing/configure.py git-cloning.fac.tum

echo bad-news >> fac-testing/README.md

${FAC:-../../fac}

grep fac-testing/configure.py git-cloning.fac.tum

cd fac-testing
git reset --hard HEAD~1
cd ..

${FAC:-../../fac}

grep fac-testing/configure.py git-cloning.fac.tum

exit 0
