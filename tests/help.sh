#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

${FAC:-../../fac} --help

exit 0
