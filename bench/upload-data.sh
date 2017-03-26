#!/bin/bash

set -ev

tar jcvf bench/fac-bench.tar.bz2 bench/data/

scp bench/fac-bench.tar.bz2 science.oregonstate.edu:public_html/
