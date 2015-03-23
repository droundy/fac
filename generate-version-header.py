#!/usr/bin/python3

import subprocess, os

name = subprocess.check_output(['git', 'describe', '--dirty'])

print('static const char *version_identifier = "%s";' % name.decode(encoding='UTF-8')[:-1])
