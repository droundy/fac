#!/bin/sh

# This test involves various build rules that depend on the contents
# of the .git directory.  I have been unsuccessfully trying to
# reproduce a bug I triggered (once) in another repository where `fac
# -c` modified the contents of .git/.

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
# without the sleep 2 below, we may run into a time-stamp race
# condition, where the output files are not included in "thestatus",
# but they exist by the time we hash the contents of the directory.
# Thus we would not rebuild thestatus when we really need to do so.

| sleep 2 && git log > thelog

# The git gc below edits the contents of the .git/ directory, and can
# cause fac to believe that these are "output" files.  This screws
# everything up when we run fac -c and fac deletes some of git's
# internal database.  A solution (maybe not the best) is to have fac
# blacklist out files in .git/ (except maybe the hooks).

| sleep 2 && git gc && git log --stat > thelogstat

| git status > thestatus
EOF

cat > README <<EOF
This test uses git to make interesting changes to output files.
EOF

cat > .gitignore <<EOF
EOF

git init
git add top.fac README .gitignore
git commit -am 'add files'

../../fac

grep 'add files' thelog
grep README thelogstat
cat thestatus

# we may need to call this a second time to make the
# status reflect all three output files.
../../fac

cat thestatus
grep thelog thestatus
grep thelogstat thestatus
grep thestatus thestatus

../../fac --clean -v

if test -e thelog; then
  echo file thelog should have been deleted
  exit 1
fi

if test -e thestatus; then
  echo file thestatus should have been deleted
  exit 1
fi

git diff

git status | grep 'working directory clean'

sleep 1
cat > junk <<EOF
junk
EOF

../../fac -j1
../../fac

grep junk thestatus
grep thelog thestatus
grep thelogstat thestatus
grep thestatus thestatus

../../fac -c

git diff

if test -e thelog; then
  echo file thelog should have been deleted
  exit 1
fi

if test -e thestatus; then
  echo file thestatus should have been deleted
  exit 1
fi

exit 0
