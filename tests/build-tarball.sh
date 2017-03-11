#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| cat foo/bar > baz

| grep nice foo/dir/bar > foo/nice

| cp foo/nice foo/nice1
< foo/nice

| cp baz baz2
< baz

| cp baz2 baz3
< baz2
EOF

mkdir foo

cat > foo/foo.fac <<EOF
| cp bar silly
EOF

echo bar > foo/bar
mkdir foo/dir
echo mean > foo/dir/bar
echo nice >> foo/dir/bar

git init
git add top.fac foo/foo.fac

../../fac --git-add

grep bar baz
grep nice foo/nice
grep bar baz3

../../fac baz3 foo/nice1

../../fac --tar fun.tar.gz --script build.sh --tupfile Tupfile --makefile Makefile

tar zxvf fun.tar.gz

cd fun
sh build.sh
grep bar baz
grep nice foo/nice

cd ..
rm -rf fun

if which make; then
    tar zxvf fun.tar.gz

    cd fun
    make
    grep bar baz
    grep nice foo/nice

    cd ..
    rm -rf fun
fi

if which tup; then
    tar zxvf fun.tar.gz

    cd fun
    cat Tupfile
    tup init
    tup
    grep bar foo/bar
    grep nice foo/dir/bar
    grep bar baz
    grep nice foo/nice

    cd ..
    rm -rf fun
fi

# the following is hokey, we wouldn't really do this
touch Tupfile.ini

../../fac -v --include-in-tar Tupfile.ini --tar fun.tar.gz --tupfile Tupfile --dotfile fun.dot

tar zxvf fun.tar.gz
cd fun
cat Tupfile.ini

if grep bar baz; then
    echo baz should not be in the tarball!
    exit 1
fi
if grep nice nice; then
    echo nice should not be in the tarball!
    exit 1
fi

if which tup; then

    ls -lh --recursive
    cat Tupfile
    tup init
    tup
    grep bar foo/bar
    grep nice foo/dir/bar
    grep bar baz
    grep nice foo/nice

    cd ..
    rm -rf fun
fi

exit 0
