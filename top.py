#!/usr/bin/python2

import string, glob

flags = '-Wall -std=c11 -g'

sources = ['bilge', 'files', 'targets', 'build']

libsources = ['listset', 'bigbrother']

for s in sources:
    print '| gcc %s -c %s.c' % (flags, s)
    print '> %s.o' % s
    print

print '| gcc -o bilge', string.join(['%s.o' % s for s in sources]), string.join(['lib/%s.o' % s for s in libsources])
for s in sources:
    print '< %s.o' % s
for s in libsources:
    print '< lib/%s.o' % s
print '> bilge'
print

for sh in glob.glob('tests/*.sh'):
    print '| sh %s > %s.log' % (sh, sh[:-3])
    print '> %s.log' % (sh[:-3])
    print
