#!/usr/bin/python3
from __future__ import print_function

import os

assert not os.system('./fac fac-afl')

assert not os.system('./fac-afl fac-afl')

os.system('mkdir -p tests/afl-input')

with open('tests/afl-input/small.fac', 'w') as f:
    f.write('''| echo good
> out1
> out2
< in1
< in2
< in3
C /usr
c .fun

# comment

| echo bad
''')

os.system('afl-fuzz -i tests/afl-input/ -o tests/afl-output -- ./fac-afl --parse-only @@')
