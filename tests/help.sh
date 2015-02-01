#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

../../fac --help

exit 0
