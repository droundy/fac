#!/bin/sh

set -ev

# This test is intended to check for stack overflow bugs.  Sadly, that
# makes it pretty slow, since it must necessarily deal with large
# numbers of rules.  It could be a bit faster if we didn't actually
# invoke gcc for each rule, but that would be tedious, and would also
# reduce the number of inputs for each rule.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

git init

python3 ../../bench/dependentchains.py 562 #  1778

ls .git

time ${FAC:-../../fac} --dry

exit 0

# Uncomment the above to have a more thorough test for more stack overflows.

time ${FAC:-../../fac}

${FAC:-../../fac} -v > fac.out
cat fac.out

if grep gcc fac.out; then
    echo we rebuilt when we should not have
    exit 1
fi

cat modifying-header.sh
sh modifying-header.sh

time ${FAC:-../../fac}


exit 0
