#!/usr/bin/python2

import string

flags = '-Wall -Werror -O2 -std=c11 -g -pg'

sources = ['bilge', 'files', 'targets', 'build']

libsources = ['trie', 'listset', 'arrayset', 'bigbrother']

for s in sources:
    print '| gcc %s -c %s.c' % (flags, s)
    print '> %s.o' % s
    print
    print '| gcc -m32 %s -o %s-32.o -c %s.c' % (flags, s, s)
    print '> %s-32.o' % s
    print

for s in libsources + ['fileaccesses']:
    print '| cd lib && gcc %s -c %s.c' % (flags, s)
    print '> lib/%s.o' % s
    print
    if s in ['bigbrother', 'fileaccesses']:
        continue
    print '| cd lib && gcc -m32 %s -o %s-32.o -c %s.c' % (flags, s, s)
    print '> lib/%s-32.o' % s
    print

print """
| python lib/get_syscalls.py /usr/src/linux-headers-3.2.0-4-common > lib/syscalls.h
> lib/syscalls.h
"""

print '| gcc -lpthread -pg -lpopt -o bilge', string.join(['%s.o' % s for s in sources] + ['lib/%s.o' % s for s in libsources])
for s in sources:
    print '< %s.o' % s
for s in libsources:
    print '< lib/%s.o' % s
print '> bilge'
print

print '| cd lib && gcc -lpthread -pg -lpopt -o fileaccesses fileaccesses.o', string.join(['%s.o' % s for s in libsources])
for s in libsources:
    print '< lib/%s.o' % s
print '> lib/fileaccesses'
print


ctests = ['arrayset', 'listset', 'trie', 'spinner']

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
