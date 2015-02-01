#!/usr/bin/python2

import string, os


os.mkdir('testing-flags');
with open('testing-flags/test.c', 'w') as f:
    f.write("""int main() {
  return 0;
}
""")
flags = ''
for flag in ['-Wall', '-Werror', '-O2', '-std=c11', '-g', '-fprofile-arcs', '-ftest-coverage']:
    if not os.system('cd testing-flags && gcc %s %s -c test.c' %
                     (flags, flag)):
        flags += ' ' + flag
    else:
        print '# gcc cannot use flag:', flag
if len(flags) > 0:
    flags = flags[1:]
linkflags = ''
for flag in ['-lpopt', '-lprofiler', '-fprofile-arcs', '-ftest-coverage']:
    if not os.system('cd testing-flags && gcc %s %s -o test test.c' %
                     (flags, flag)):
        linkflags += ' ' + flag
    else:
        print '# gcc linking cannot use flag:', flag
if len(linkflags) > 0:
    linkflags = linkflags[1:]


flags32 = '-m32'
for flag in ['-Wall', '-Werror', '-O2', '-std=c11', '-g', '-fprofile-arcs', '-ftest-coverage']:
    if not os.system('cd testing-flags && gcc %s %s -c test.c' %
                     (flags32, flag)):
        flags32 += ' ' + flag
    else:
        print '# gcc cannot use flag:', flag
linkflags32 = '-m32'
for flag in ['-lpopt', '-lprofiler', '-fprofile-arcs', '-ftest-coverage']:
    if not os.system('cd testing-flags && gcc %s %s -o test test.c' %
                     (flags32, flag)):
        linkflags32 += ' ' + flag
    else:
        print '# gcc linking cannot use flag:', flag
os.system('rm -rf testing-flags')

sources = ['fac', 'files', 'targets', 'clean', 'new-build', 'git']

libsources = ['listset', 'iterablehash', 'arrayset', 'bigbrother']

for s in sources:
    print '| gcc %s -c %s.c' % (flags, s)
    print '> %s.o' % s
    print
    print '| gcc %s -o %s-32.o -c %s.c' % (flags32, s, s)
    print '> %s-32.o' % s
    print

for s in libsources + ['fileaccesses']:
    print '| cd lib && gcc %s -c %s.c' % (flags, s)
    print '> lib/%s.o' % s
    if s in ['bigbrother']:
        print '< lib/syscalls.h'
    print
    if s in ['bigbrother', 'fileaccesses']:
        continue
    print '| cd lib && gcc %s -o %s-32.o -c %s.c' % (flags32, s, s)
    print '> lib/%s-32.o' % s
    print

print """
| python lib/get_syscalls.py /usr/src/linux-headers-3.2.0-4-common > lib/syscalls.h
> lib/syscalls.h
"""

print '| gcc '+linkflags+' -o fac', string.join(['%s.o' % s for s in sources] + ['lib/%s.o' % s for s in libsources])
for s in sources:
    print '< %s.o' % s
for s in libsources:
    print '< lib/%s.o' % s
print '> fac'
print

print '| cd lib && gcc '+linkflags+' -o fileaccesses fileaccesses.o', string.join(['%s.o' % s for s in libsources])
for s in libsources + ['fileaccesses']:
    print '< lib/%s.o' % s
print '> lib/fileaccesses'
print


ctests = ['arrayset', 'listset', 'spinner', 'iterable_hash_test']

for test in ctests:
    print '| gcc '+linkflags+' -o tests/%s.test' % test, 'tests/%s.o' % test, string.join(['lib/%s.o' % s for s in libsources])
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
    print '| gcc '+linkflags32+' -o tests/%s-32.test' % test, 'tests/%s-32.o' % test, string.join(['lib/%s-32.o' % s for s in libsources])
    print '> tests/%s-32.test' % test
    print '< tests/%s-32.o' % test
    for s in libsources:
        print '< lib/%s-32.o' % s
    print

    print '| cd tests && gcc %s -o %s-32.o -c %s.c' % (flags32, test, test)
    print '> tests/%s-32.o' % test
    print
