#!/bin/sh

set -ev

echo $0

# This test ensures that we can build fac properly from the git
# source.  Sadly that means it doesn't test the uncommitted changes.
# But at least I should have some confirmation that the build script
# works right!

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

# travis gets a shallow clone, which I can't clone, so I will clone
# the old-fashioned way...

tarname=`ls ../../fac-*.tar.gz | tail -1`

echo tarname is $tarname
tar xvzf $tarname

cd fac-*

ls -lh
cat build/linux.sh
sh build/linux.sh

cat README.md

exit 0
