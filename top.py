#!/usr/bin/python2

import string, os, sys

os.mkdir('testing-flags');
with open('testing-flags/test.c', 'w') as f:
    f.write("""int main() {
  return 0;
}
""")

# add , '-fprofile-arcs', '-ftest-coverage' to both of the following
# lines in order to enable gcov coverage testing
possible_flags = ['-Wall', '-Werror', '-O2', '-std=c11', '-std=c99', '-g']
possible_linkflags = ['-lpopt', '-lpthread', '-lprofiler']

cc = os.getenv('CC', 'gcc')
flags = os.getenv('CFLAGS', '')
linkflags = os.getenv('LDFLAGS', '')

def compile_works(flags):
    return not os.system('cd testing-flags && %s %s -c test.c' % (cc, flags))
def link_works(flags):
    return not os.system('cd testing-flags && %s %s -o test test.c' % (cc, flags))

if not compile_works(flags):
    sys.stderr.write('unable to compile using %s %s -c test.c' % (cc, flags))
    os.system('rm -rf testing-flags')
    exit(1)
if not link_works(linkflags):
    sys.stderr.write('unable to link using %s %s -o test test.c' % (cc, linkflags))
    os.system('rm -rf testing-flags')
    exit(1)

for flag in possible_flags:
    if compile_works(flags+' '+flag):
        flags += ' ' + flag
    else:
        print '# %s cannot use flag:' % cc, flag
if len(flags) > 0 and flags[0] == ' ':
    flags = flags[1:]
for flag in possible_linkflags:
    if link_works(linkflags + ' ' + flag):
        linkflags += ' ' + flag
    else:
        print '# %s linking cannot use flag:' % cc, flag
if len(linkflags) > 0 and linkflags[0] == ' ':
    linkflags = linkflags[1:]

flags32 = '-m32 ' + os.getenv('CFLAGS', '')
linkflags32 = '-m32 ' + os.getenv('LDFLAGS', '')
compile32 = compile_works(flags32) and link_works(linkflags32)
if compile32:
    for flag in possible_flags:
        if not os.system('cd testing-flags && %s %s %s -c test.c' %
                         (cc, flags32, flag)):
            flags32 += ' ' + flag
        else:
            print '# %s 32-bit cannot use flag:' % cc, flag
    for flag in possible_linkflags:
        if not os.system('cd testing-flags && %s %s %s -o test test.c' %
                         (cc, linkflags32, flag)):
            linkflags32 += ' ' + flag
        else:
            print '# %s 32-bit linking cannot use flag:' % cc, flag
os.system('rm -rf testing-flags')

sources = ['fac', 'files', 'targets', 'clean', 'new-build', 'git', 'environ']

libsources = ['listset', 'iterablehash', 'arrayset', 'bigbrother', 'sha1',
              'hashset']

for s in sources:
    print '| %s %s -c %s.c' % (cc, flags, s)
    print '> %s.o' % s
    print
    if compile32:
        print '| %s %s -o %s-32.o -c %s.c' % (cc, flags32, s, s)
        print '> %s-32.o' % s
        print

for s in libsources + ['fileaccesses']:
    print '| cd lib && %s %s -c %s.c' % (cc, flags, s)
    print '> lib/%s.o' % s
    if s in ['bigbrother']:
        print '< lib/syscalls.h'
    print
    if s in ['bigbrother', 'fileaccesses']:
        continue
    if compile32:
        print '| cd lib && %s %s -o %s-32.o -c %s.c' % (cc, flags32, s, s)
        print '> lib/%s-32.o' % s
        print

print """
| python lib/get_syscalls.py /usr/src/linux-headers-3.2.0-4-common > lib/syscalls.h
> lib/syscalls.h
"""

print '| %s '%cc+linkflags+' -o fac', string.join(['%s.o' % s for s in sources] + ['lib/%s.o' % s for s in libsources])
for s in sources:
    print '< %s.o' % s
for s in libsources:
    print '< lib/%s.o' % s
print '> fac'
print

print '| cd lib && %s '%cc+linkflags+' -o fileaccesses fileaccesses.o', string.join(['%s.o' % s for s in libsources])
for s in libsources + ['fileaccesses']:
    print '< lib/%s.o' % s
print '> lib/fileaccesses'
print


ctests = ['arrayset', 'hashset', 'listset', 'spinner', 'iterable_hash_test']

for test in ctests:
    print '| %s '%cc+linkflags+' -o tests/%s.test' % test, 'tests/%s.o' % test, string.join(['lib/%s.o' % s for s in libsources])
    print '> tests/%s.test' % test
    print '< tests/%s.o' % test
    for s in libsources:
        print '< lib/%s.o' % s
    print

    print '| cd tests && %s %s -c %s.c' % (cc, flags, test)
    print '> tests/%s.o' % test
    print

if compile32:
    libsources.remove('bigbrother') # it doesn't work yet with 32 bit

    for test in ctests:
        print '| %s '%cc+linkflags32+' -o tests/%s-32.test' % test, 'tests/%s-32.o' % test, string.join(['lib/%s-32.o' % s for s in libsources])
        print '> tests/%s-32.test' % test
        print '< tests/%s-32.o' % test
        for s in libsources:
            print '< lib/%s-32.o' % s
        print

        print '| cd tests && %s %s -o %s-32.o -c %s.c' % (cc, flags32, test, test)
        print '> tests/%s-32.o' % test
        print
