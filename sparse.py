#!/usr/bin/python3

from __future__ import print_function
import string, os, sys, platform, subprocess

myplatform = sys.platform
if myplatform == 'linux2':
    myplatform = 'linux'

if myplatform != 'linux':
    print('# cannot use sparse on platform', myplatform)
    exit(0)

ver = subprocess.run(['sparse', '--version'],
                     stderr=subprocess.STDOUT,
                     stdout=subprocess.PIPE)

if ver.returncode != 0:
    print('# there is no sparse present')
    exit(0)

myver = b'v0.5.0-'
if ver.stdout[:len(myver)] == myver:
    print('# looking good!')
else:
    print('# I am not confident with sparse version', ver.stdout)

files_to_sparse = ['fac.c', 'environ.c', 'clean-all.c', 'arguments.c', 'git.c',
                   'build.c']

for f in files_to_sparse:
    print('| sparse -Ibigbro -Wsparse-error %s > %s.sparse' % (f, f[:-2]))
    print('> %s.sparse' % (f[:-2]))
