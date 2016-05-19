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

git clone ../.. new-fac

cd new-fac

sh build-linux.sh

./fac

exit 0
