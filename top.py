#!/usr/bin/python2

import string

flags = '-Wall -Werror -O2 -std=c11 -g'

sources = ['bilge', 'files', 'targets', 'build']

libsources = ['listset', 'arrayset', 'bigbrother']

for s in sources:
    print '| gcc %s -c %s.c' % (flags, s)
    print '> %s.o' % s
    print
    print '| gcc -m32 %s -o %s-32.o -c %s.c' % (flags, s, s)
    print '> %s-32.o' % s
    print

print '| gcc -lpthread -lpopt -o bilge', string.join(['%s.o' % s for s in sources]), string.join(['lib/%s.o' % s for s in libsources])
for s in sources:
    print '< %s.o' % s
for s in libsources:
    print '< lib/%s.o' % s
print '> bilge'
print


ctests = ['arrayset', 'listset', 'spinner']

for test in ctests:
    print '| gcc -lpthread -o tests/%s.test' % test, 'tests/%s.o' % test, string.join(['lib/%s.o' % s for s in libsources])
    print '> tests/%s.test' % test
    print '< tests/%s.o' % test
    for s in libsources:
        print '< lib/%s.o' % s
    print

    print '| cd tests && gcc %s -c %s.c' % (flags, test)
    print '> tests/%s.o' % test
    print

ctests = ['arrayset', 'listset', 'spinner']

libsources.remove('bigbrother') # it doesn't work yet with 32 bit

for test in ctests:
    print '| gcc -m32 -lpthread -o tests/%s-32.test' % test, 'tests/%s-32.o' % test, string.join(['lib/%s-32.o' % s for s in libsources])
    print '> tests/%s-32.test' % test
    print '< tests/%s-32.o' % test
    for s in libsources:
        print '< lib/%s-32.o' % s
    print

    print '| cd tests && gcc -m32 %s -o %s-32.o -c %s.c' % (flags, test, test)
    print '> tests/%s-32.o' % test
    print
