#!/usr/bin/python2

import string, glob

flags = '-Wall -std=c11 -g'

sources = ['bilge', 'files', 'targets', 'build']

libsources = ['listset', 'arrayset', 'bigbrother']

for s in sources:
    print '| gcc %s -c %s.c' % (flags, s)
    print '> %s.o' % s
    print

print '| gcc -lpthread -o bilge', string.join(['%s.o' % s for s in sources]), string.join(['lib/%s.o' % s for s in libsources])
for s in sources:
    print '< %s.o' % s
for s in libsources:
    print '< lib/%s.o' % s
print '> bilge'
print


ctests = ['arrayset']

for test in ctests:
    print '| gcc -lpthread -o tests/%s.test' % test, 'tests/%s.o' % test, string.join(['lib/%s.o' % s for s in libsources])
    print '> tests/%s.test' % test
    print '< tests/%s.o' % test
    for s in libsources:
        print '< lib/%s.o' % s
    print

    print '| cd tests && gcc %s -c %s.c' % (flags, test)
    print '> tests/%s.o' % s
    print
