#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

git init

cat > git-cloning.fac <<EOF
| if cd fac-testing; then git pull; else git clone ../.. fac-testing; fi
> fac-testing/configure.fac
> fac-testing/bigbro/syscalls.fac
C fac-testing/.git/ref/
EOF
git add git-cloning.fac

../../fac

grep superheros-are-awesome fac-testing/tests/git-clone-rule.sh

grep fac-testing/lib/master.py git-cloning.fac.tum

echo bad-news >> fac-testing/tests/git-clone-rule.sh

../../fac

grep fac-testing/lib/master.py git-cloning.fac.tum

cd fac-testing
git reset --hard HEAD~1
cd ..

../../fac

grep fac-testing/lib/master.py git-cloning.fac.tum

exit 0
