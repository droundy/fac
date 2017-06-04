#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

git init

python3 ../../bench/dependentchains.py 1778

ls .git

time ${FAC:-../../fac} --dry

# ${FAC:-../../fac} -v > fac.out
# cat fac.out

# if grep gcc fac.out; then
#     echo we rebuilt when we should not have
#     exit 1
# fi

exit 0
