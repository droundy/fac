#!/usr/bin/python2

import string, os, sys, platform

os.system('rm -rf testing-flags')
os.mkdir('testing-flags');
with open('testing-flags/test.c', 'w') as f:
    f.write("""int main() {
  return 0;
}
""")

# add , '-fprofile-arcs', '-ftest-coverage' to both of the following
# lines in order to enable gcov coverage testing
optional_flags = ['-Wall', '-Werror', '-O2', '-g']
optional_linkflags = ['-lprofiler']

possible_flags = ['-std=c11', '-std=c99']
possible_linkflags = ['-lpopt', '-lpthread']

if os.getenv('MINIMAL','') == '':
    print '# We are not minimal'
    possible_flags += optional_flags
    possible_linkflags += optional_linkflags
else:
    print '# We are minimal'

cc = os.getenv('CC', 'gcc')
flags = [os.getenv('CFLAGS', '')]
linkflags = [os.getenv('LDFLAGS', '')]

def compile_works(flags):
    return not os.system('cd testing-flags && %s %s -c test.c' % (cc, ' '.join(flags)))
def link_works(flags):
    return not os.system('cd testing-flags && %s %s -o test test.c' % (cc, ' '.join(flags)))

if not compile_works(flags):
    sys.stderr.write('unable to compile using %s %s -c test.c' % (cc, flags))
    os.system('rm -rf testing-flags')
    exit(1)
if not link_works(linkflags):
    sys.stderr.write('unable to link using %s %s -o test test.c' % (cc, linkflags))
    os.system('rm -rf testing-flags')
    exit(1)

for flag in possible_flags:
    if compile_works(flags+[flag]):
        flags += [flag]
    else:
        print '# %s cannot use flag:' % cc, flag
if len(flags) > 0 and flags[0] == ' ':
    flags = flags[1:]
for flag in possible_linkflags:
    if link_works(linkflags + [flag]):
        linkflags += [flag]
    else:
        print '# %s linking cannot use flag:' % cc, flag

if '-std=c11' in flags:
    flags = [f for f in flags if f != '-std=c99']
linkflags = filter(None, linkflags)
flags = filter(None, flags)

flags32 = ['-m32', os.getenv('CFLAGS', '')]
linkflags32 = ['-m32', os.getenv('LDFLAGS', '')]
compile32 = compile_works(flags32) and link_works(linkflags32)
if compile32:
    for flag in possible_flags:
        if compile_works(flags32+[flag]):
            flags32 += [flag]
        else:
            print '# %s 32-bit cannot use flag:' % cc, flag
    for flag in possible_linkflags:
        if link_works(linkflags32+[flag]):
            linkflags32 += [flag]
        else:
            print '# %s 32-bit linking cannot use flag:' % cc, flag
os.system('rm -rf testing-flags')

if '-std=c11' in flags32:
    flags32 = [f for f in flags32 if f != '-std=c99']
flags32 = filter(None, flags32)
linkflags32 = filter(None, linkflags32)

sources = ['fac', 'files', 'targets', 'clean', 'new-build', 'git', 'environ']

libsources = ['listset', 'iterablehash', 'sha1', 'hashset', 'posixmodel']

for s in sources:
    print '| %s %s -c %s.c' % (cc, ' '.join(flags), s)
    print '> %s.o' % s
    if s == 'fac':
        print '< version-identifier.h'
    print
    if compile32:
        print '| %s %s -o %s-32.o -c %s.c' % (cc, ' '.join(flags32), s, s)
        print '> %s-32.o' % s
        print


extra_libs = ['bigbrotheralt', 'fileaccesses']
if platform.system() == 'Linux':
    extra_libs += ['bigbrother']

for s in libsources + extra_libs:
    print '| cd lib && %s %s -c %s.c' % (cc, ' '.join(flags), s)
    print '> lib/%s.o' % s
    if s in ['bigbrother', 'bigbrotheralt']:
        print '< lib/linux-syscalls.h'
        print '< lib/freebsd-syscalls.h'
    print
    if s in ['fileaccesses', 'fileaccessesalt']:
        continue
    if compile32:
        print '| cd lib && %s %s -o %s-32.o -c %s.c' % (cc, ' '.join(flags32), s, s)
        print '> lib/%s-32.o' % s
        print

print '| %s '%cc+' '.join(linkflags)+' -o fac', string.join(['%s.o' % s for s in sources] + ['lib/%s.o' % s for s in libsources+['bigbrotheralt']])
for s in sources:
    print '< %s.o' % s
for s in libsources+['bigbrotheralt']:
    print '< lib/%s.o' % s
print '> fac'
print

print '| cd lib && %s '%cc+' '.join(linkflags)+' -o fileaccesses fileaccesses.o', string.join(['%s.o' % s for s in libsources+['bigbrotheralt']])
for s in libsources + ['fileaccesses', 'bigbrotheralt']:
    print '< lib/%s.o' % s
print '> lib/fileaccesses'
print

if platform.system() == 'Linux':
    print '| %s '%cc+' '.join(linkflags)+' -o altfac', string.join(['%s.o' % s for s in sources] + ['lib/%s.o' % s for s in libsources+['bigbrother']])
    for s in sources:
        print '< %s.o' % s
    for s in libsources+['bigbrother']:
        print '< lib/%s.o' % s
    print '> altfac'
    print

    print '| cd lib && %s '%cc+' '.join(linkflags)+' -o fileaccessesalt fileaccesses.o', string.join(['%s.o' % s for s in libsources+['bigbrother']])
    for s in libsources + ['fileaccesses', 'bigbrother']:
        print '< lib/%s.o' % s
    print '> lib/fileaccessesalt'
    print


ctests = ['hashset', 'listset', 'spinner', 'iterable_hash_test', 'assertion-fails',
          'test-posix-model']

for test in ctests:
    print '| %s '%cc+' '.join(linkflags)+' -o tests/%s.test' % test, 'tests/%s.o' % test, string.join(['lib/%s.o' % s for s in libsources])
    print '> tests/%s.test' % test
    print '< tests/%s.o' % test
    for s in libsources:
        print '< lib/%s.o' % s
    print

    print '| cd tests && %s %s -c %s.c' % (cc, ' '.join(flags), test)
    print '> tests/%s.o' % test
    print

if compile32:
    for test in ctests:
        print '| %s '%cc+' '.join(linkflags32)+' -o tests/%s-32.test' % test, 'tests/%s-32.o' % test, string.join(['lib/%s-32.o' % s for s in libsources])
        print '> tests/%s-32.test' % test
        print '< tests/%s-32.o' % test
        for s in libsources:
            print '< lib/%s-32.o' % s
        print

        print '| cd tests && %s %s -o %s-32.o -c %s.c' % (cc, ' '.join(flags32), test, test)
        print '> tests/%s-32.o' % test
        print
