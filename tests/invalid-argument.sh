#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

git init

if ${FAC:-../../fac} -z; then
    echo fac with an invalid argument should have failed.
    exit 1
fi

if ${FAC:-../../fac} --z; then
    echo fac with an invalid argument should have failed.
    exit 1
fi

exit 0
