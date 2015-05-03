#!/usr/bin/python3

from __future__ import print_function
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
possible_linkflags = ['-lpopt', '-lpthread', '-lm']

if os.getenv('MINIMAL') == None:
    print('# We are not minimal')
    possible_flags += optional_flags
    possible_linkflags += optional_linkflags

if os.getenv('MINIMAL') == None and os.getenv('CC') == None and os.getenv('CFLAGS') == None and os.getenv('LDFLAGS') == None:
    # No special configuration was specified, so let us try to see how
    # many variants we can build.
    variants = {'': {'cc': 'gcc',
                     'ar': 'ar',
                     'ranlib': 'ranlib',
                     'flags': [],
                     'linkflags': [],
                     'os': platform.system().lower(),
                     'arch': platform.machine()},
                '32': {'cc': 'gcc',
                       'ar': 'ar',
                       'ranlib': 'ranlib',
                       'flags': ['-m32'],
                       'linkflags': ['-m32'],
                       'os': platform.system().lower(),
                       'arch': 'i386'},
                'clang': {'cc': 'clang',
                          'ar': 'ar',
                          'ranlib': 'ranlib',
                          'flags': [],
                          'linkflags': [],
                          'os': platform.system().lower(),
                          'arch': platform.machine()},
                'darwin': {'cc': 'x86_64-apple-darwin10-clang',
                           'ar': 'x86_64-apple-darwin10-ar',
                           'ranlib': 'x86_64-apple-darwin10-ranlib',
                           'flags': ['-I'+os.path.join(os.getcwd(),'../darwin')],
                           'linkflags': ['-L'+os.path.join(os.getcwd(),'../darwin')],
                           'os': 'darwin',
                           'arch': 'x86_64'},
                'w64': {'cc': 'x86_64-w64-mingw32-gcc',
                        'ar': 'x86_64-w64-mingw32-ar',
                        'ranlib': 'x86_64-w64-mingw32-ranlib',
                        'flags': ['-I'+os.path.join(os.getcwd(),'../win32')],
                        'linkflags': ['-I'+os.path.join(os.getcwd(),'../win32')],
                        'os': 'windows',
                        'arch': 'x86_64'}}
else:
    if os.getenv('MINIMAL') == None:
        print('# We are not minimal')
        variants = {'': {'cc': os.getenv('CC', 'gcc'),
                         'flags': [os.getenv('CFLAGS', '')],
                         'linkflags': [os.getenv('LDFLAGS', '')],
                         'os': platform.system().lower(),
                         'arch': platform.machine()}}
    else:
        print('# We are minimal')
        possible_flags.remove('-std=c11')
        cc = os.getenv('CC', 'oopsies')
        variants = {'': {'cc': os.getenv('CC', '${CC-gcc}'),
                         'flags': [os.getenv('CFLAGS', '${CFLAGS-}')],
                         'linkflags': [os.getenv('LDFLAGS', '${LDFLAGS-}')],
                         'os': platform.system().lower(),
                         'arch': platform.machine()}}

    print('# compiling with just the variant:', variants)

def compile_works(flags):
    return not os.system('cd testing-flags && %s %s -c test.c' % (cc, ' '.join(flags)))
def link_works(flags):
    print('# trying',
          ('cd testing-flags && %s %s -o test test.c' % (cc, ' '.join(flags))),
          file=sys.stdout)
    return not os.system('cd testing-flags && %s %s -o test test.c' % (cc, ' '.join(flags)))

for variant in variants.keys():
    print('# Considering variant: "%s"' % variant)
    cc = variants[variant]['cc']
    flags = variants[variant]['flags']
    linkflags = variants[variant]['linkflags']
    variant_name = '-'+variant
    if variant == '':
        variant_name = ''
    else:
        os.makedirs('build/'+variant, exist_ok=True)
        print("""
# | cd build/%s && rm -rf bigbro && git clone git://github.com/droundy/bigbro
# > build/%s/bigbro/bigbro-linux.c
        """ % (variant, variant))

    if not compile_works(flags):
        print('# unable to compile using %s %s -c test.c' % (cc, flags))
        continue
    if not link_works(linkflags):
        print('# unable to link using %s %s -o test test.c\n' % (cc, ' '.join(linkflags)))
        continue

    for flag in possible_flags:
        if compile_works(flags+[flag]):
            flags += [flag]
        else:
            print('# %s%s cannot use flag:' % (cc, variant_name), flag)
    if len(flags) > 0 and flags[0] == ' ':
        flags = flags[1:]
    for flag in possible_linkflags:
        if link_works(linkflags + [flag]):
            linkflags += [flag]
        else:
            print('# %s%s linking cannot use flag:' % (cc, variant_name), flag)

    if '-std=c11' in flags:
        flags = [f for f in flags if f != '-std=c99']
    linkflags = list(filter(None, linkflags))
    flags = list(filter(None, flags))

    variants[variant]['flags'] = flags
    variants[variant]['linkflags'] = linkflags

    sources = ['fac', 'files', 'targets', 'clean', 'new-build', 'git', 'environ']

    libsources = ['listset', 'iterablehash', 'intmap', 'sha1', 'hashset', 'posixmodel']

    for s in sources:
        print('| %s %s -o %s%s.o -c %s.c' % (cc, ' '.join(flags), s, variant_name, s))
        print('> %s%s.o' % (s, variant_name))
        if s == 'fac':
            print('< version-identifier.h')
        print()

    extra_libs = ['bigbrother', 'fileaccesses']

    if variant in ['w64', 'darwin']:
        extra_libs.remove('bigbrother')

    for s in libsources + extra_libs:
        print('| cd lib && %s %s -o %s%s.o -c %s.c' % (cc, ' '.join(flags), s, variant_name, s))
        print('> lib/%s%s.o' % (s, variant_name))
        if s in ['bigbrother']:
            print('< lib/linux-syscalls.h')
            print('< lib/freebsd-syscalls.h')
        print()
        if s in ['fileaccesses', 'fileaccessesalt']:
            continue

    if '-lpopt' not in linkflags or 'bigbrother' not in extra_libs:
        print('# this is all we can do with %s so far' % variant)
        continue

    print('| %s -o fac%s %s' %
          (cc, variant_name,
           ' '.join(['%s%s.o' % (s, variant_name) for s in sources]
                    + ['lib/%s%s.o' % (s, variant_name) for s in libsources+['bigbrother']]
                    + linkflags)))
    for s in sources:
        print('< %s%s.o' % (s, variant_name))
    for s in libsources+['bigbrother']:
        print('< lib/%s%s.o' % (s, variant_name))
    print('> fac%s' % variant_name)
    print()

    print('| %s -o lib/fileaccesses%s lib/fileaccesses%s.o %s' %
          (cc, variant_name, variant_name,
           ' '.join(['lib/%s%s.o' % (s, variant_name) for s in libsources+['bigbrother']]
                    + linkflags)))
    for s in libsources + ['fileaccesses', 'bigbrother']:
        print('< lib/%s%s.o' % (s, variant_name))
    print('> lib/fileaccesses%s' % variant_name)
    print()

    ctests = ['hashset', 'listset', 'spinner', 'iterable_hash_test', 'assertion-fails',
              'test-posix-model']

    for test in ctests:
        print('| %s '%cc+' '.join(linkflags)+' -o tests/%s%s.test' % (test, variant_name),
              'tests/%s%s.o' % (test, variant_name),
              ' '.join(['lib/%s%s.o' % (s, variant_name) for s in libsources]))
        print('> tests/%s%s.test' % (test, variant_name))
        print('< tests/%s%s.o' % (test, variant_name))
        for s in libsources:
            print('< lib/%s%s.o' % (s, variant_name))
        print()

        print('| cd tests && %s %s -o %s%s.o -c %s.c'
              % (cc, ' '.join(flags), test, variant_name, test))
        print('> tests/%s%s.o' % (test, variant_name))
        print()

os.system('rm -rf testing-flags')
