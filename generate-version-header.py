#!/usr/bin/python3

import subprocess, os

try:
    name = subprocess.check_output(['git', 'describe', '--dirty'])
except:
    name = subprocess.check_output(['git', 'rev-parse', 'HEAD'])

print('static const char *version_identifier = "%s";' % name.decode(encoding='UTF-8')[:-1])
