#!/bin/sh

set -ev

echo $0

# This test ensures that we can count builds properly to figure out if
# we have built everything correctly.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

mkdir subdir

for i in `seq 1 48`; do
    cat >> top.fac <<EOF
| sleep 5 && echo file_$i > file_$i.dat

EOF
done

git init
git add top.fac

../../fac -j4 > fac.out
perl -i -pe 's/\r/\n/' fac.out
cat fac.out

grep "Build time remaining: 0:0. / 1:0" fac.out
grep "Build succeeded! 1:00" fac.out

../../fac -j4 > fac.out
perl -i -pe 's/\r/\n/' fac.out
cat fac.out

grep "Build succeeded! 0:00" fac.out

rm *.dat

../../fac -j4 > fac.out
perl -i -pe 's/\r/\n/' fac.out
cat fac.out

grep "Build time remaining: 1:0. / 1:0" fac.out
grep "Build time remaining: 0:05 / 1:0" fac.out
grep "Build succeeded! 1:00" fac.out

rm file_1.dat file_2.dat file_3.dat file_4.dat
../../fac -j4 > fac.out
cat fac.out

grep 1/4 fac.out
grep "Build time remaining: 0:05 / 0:10" fac.out
grep "Build succeeded! 0:05" fac.out

rm *.dat

../../fac -j8 > fac.out
cat fac.out

grep "Build time remaining: 0:3. / 0:3" fac.out
grep "Build time remaining: 0:0. / 0:3" fac.out
grep "Build succeeded! 0:30" fac.out

exit 0
