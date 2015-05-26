#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

git init

cat > git-cloning.fac <<EOF
# | git clone git://github.com/droundy/fac.git fac-testing
| git clone ../.. fac-testing
> fac-testing/top.fac
EOF
git add git-cloning.fac

../../fac

# grep superheros-are-awesome fac-testing/tests/git-clone-rule.sh

exit 0
