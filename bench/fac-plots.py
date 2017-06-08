#!/usr/bin/python3
from __future__ import print_function

try:
    import matplotlib
except:
    print('# not generating plots due to lack of matplotlib')
    exit(0)

import cats
import hierarchy
import dependentchains
import sleepy
import independent

modules = [dependentchains, cats, hierarchy, sleepy, independent]

for mod in modules:
    for verb in mod.verbs:
        print('| python3 plot-benchmark.py %s %s' % (mod.name, verb))
        print('< data')
        print('c .pyc')
