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
mkdir new-fac
cd new-fac
cp -r ../../../.git .
git checkout .

sh build/linux.sh

./fac

exit 0
