#!/usr/bin/python3

import subprocess, os

try:
    name = subprocess.check_output(['git', 'describe', '--dirty'])
except:
    name = subprocess.check_output(['git', 'rev-parse', 'HEAD'])

version = name.decode(encoding='UTF-8')[:-1]
with open("version-identifier.h", "w") as f:
    f.write('static const char *version_identifier = "%s";\n' % version)
