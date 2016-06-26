#!/usr/bin/python3

from __future__ import print_function
import string, os, sys, platform

myplatform = sys.platform
if myplatform == 'linux2':
    myplatform = 'linux'

have_sass = os.system('sass -h > /dev/null') == 0
have_help2man = os.system('help2man --help > /dev/null') == 0
have_checkinstall = os.system('checkinstall --version > /dev/null') == 0

os.system('rm -rf testing-flags')
os.mkdir('testing-flags');
with open('testing-flags/test.c', 'w') as f:
    f.write("""
int main() {
  return 0;
}
""")
with open('testing-flags/have-popt.c', 'w') as f:
    f.write("""
#include <popt.h>

int main() {
  return 0;
}
""")

# add , '-fprofile-arcs', '-ftest-coverage' to both of the following
# lines in order to enable gcov coverage testing
optional_flags = ['-Wall', '-Werror', '-O2']
optional_linkflags = ['-lprofiler']

possible_flags = ['-std=c11', '-std=c99']
possible_linkflags = ['-lpopt', '-lpthread', '-lm']

if os.getenv('MINIMAL') == None:
    print('# We are not minimal')
    possible_flags += optional_flags
    possible_linkflags += optional_linkflags

if os.getenv('MINIMAL') == None:
    print('# We are not minimal')
    variants = {'': {'cc': os.getenv('CC', 'gcc'),
                     'flags': [os.getenv('CFLAGS', '') + ' -Ibigbro', '-g'],
                     'linkflags': [os.getenv('LDFLAGS', '')],
                     'os': platform.system().lower(),
                     'arch': platform.machine()},
                '-static': {'cc': os.getenv('CC', 'gcc'),
                            'flags': [os.getenv('CFLAGS', '') + ' -Ibigbro'],
                            'linkflags': [os.getenv('LDFLAGS', '')],
                            'os': platform.system().lower(),
                            'arch': platform.machine()},
                '-win': {'cc': 'x86_64-w64-mingw32-gcc',
                         'flags': ['-Ibigbro'],
                         'linkflags': [''],
                         'os': 'win32',
                         'arch': 'amd64'}}
else:
    print('# We are minimal')
    possible_flags.remove('-std=c11')
    cc = os.getenv('CC', 'oopsies')
    variants = {'': {'cc': os.getenv('CC', 'gcc'),
                     'flags': [os.getenv('CFLAGS', '${CFLAGS} -Ibigbro')],
                     'linkflags': [os.getenv('LDFLAGS', '${LDFLAGS-}')],
                     'os': platform.system().lower(),
                     'arch': platform.machine()}}

    print('# compiling with just the variant:', variants)

def compile_works(flags):
    return not os.system('%s %s -c -o testing-flags/test.o testing-flags/test.c' % (cc, ' '.join(flags)))
def have_popt(flags):
    return not os.system('%s %s -c -o testing-flags/test.o testing-flags/have-popt.c' % (cc, ' '.join(flags)))
def link_works(flags):
    cmd = '%s -o testing-flags/test testing-flags/test.c %s' % (cc, ' '.join(flags))
    print('# trying', cmd, file=sys.stdout)
    return not os.system(cmd)

for variant in variants.keys():
    print('# Considering variant: "%s"' % variant)
    cc = variants[variant]['cc']
    flags = variants[variant]['flags']
    linkflags = variants[variant]['linkflags']

    if not compile_works(flags):
        print('# unable to compile using %s %s -c test.c' % (cc, ' '.join(flags)))
        continue
    if not link_works(linkflags):
        print('# unable to link using %s %s -o test test.c\n' % (cc, ' '.join(linkflags)))
        continue

    for flag in possible_flags:
        if compile_works(flags+[flag]):
            flags += [flag]
        else:
            print('# %s%s cannot use flag:' % (cc, variant), flag)
    if len(flags) > 0 and flags[0] == ' ':
        flags = flags[1:]
    for flag in possible_linkflags:
        if link_works(linkflags + [flag]):
            linkflags += [flag]
        else:
            print('# %s%s linking cannot use flag:' % (cc, variant), flag)

    if '-std=c11' in flags:
        flags = [f for f in flags if f != '-std=c99']
    linkflags = list(filter(None, linkflags))
    flags = list(filter(None, flags))

    variants[variant]['flags'] = flags
    variants[variant]['linkflags'] = linkflags

    sources = ['fac', 'files', 'targets', 'clean-all', 'build', 'git', 'environ', 'mkdir']

    libsources = ['listset', 'iterablehash', 'intmap', 'sha1']

    for s in sources:
        if s == 'fac' and not have_popt(flags):
            print('# unable to use popt with %s, therefore skipping fac' %cc)
            continue

        print('| %s %s -o %s%s.o -c %s.c' % (cc, ' '.join(flags), s, variant, s))
        print('> %s%s.o' % (s, variant))
        if s == 'fac':
            print('< version-identifier.h')
        print()

    for s in libsources:
        print('| cd lib && %s %s -o %s%s.o -c %s.c' % (cc, ' '.join(flags), s, variant, s))
        print('> lib/%s%s.o' % (s, variant))
        print()

    if '-lpopt' not in linkflags:
        print('# this is all we can do with %s so far' % variant)
        exit(0)

    ctests = ['listset', 'spinner', 'iterable_hash_test', 'assertion-fails']

    for test in ctests:
        print('| %s '%cc+' '.join(linkflags)+' -o tests/%s%s.test' % (test, variant),
              'tests/%s%s.o' % (test, variant),
              ' '.join(['lib/%s%s.o' % (s, variant) for s in libsources]))
        print('> tests/%s%s.test' % (test, variant))
        print('< tests/%s%s.o' % (test, variant))
        for s in libsources:
            print('< lib/%s%s.o' % (s, variant))
        print()

        print('| cd tests && %s %s -o %s%s.o -c %s.c'
              % (cc, ' '.join(flags), test, variant, test))
        print('> tests/%s%s.o' % (test, variant))
        print()

    def build_fac(postfix=''):
        print('| %s -o fac%s%s %s' %
              (cc, variant, postfix,
               ' '.join(['%s%s.o' % (s, variant) for s in sources]
                        + ['bigbro/bigbro-%s.o' % myplatform]
                        + ['lib/%s%s.o' % (s, variant) for s in libsources]
                        + linkflags)))
        print('< bigbro/bigbro-%s.o' % myplatform)
        for s in sources:
            print('< %s%s.o' % (s, variant))
        for s in libsources:
            print('< lib/%s%s.o' % (s, variant))
        print('> fac%s%s' % (variant, postfix))
        print()

    build_fac()
    # if variant == '':
    #     linkflags = ['-static']+linkflags
    #     build_fac('-static')
    #     linkflags = linkflags[1:]

os.system('rm -rf testing-flags')

if have_checkinstall and have_help2man:
    print('''
| sh build/deb.sh
> web/fac-latest.deb
< fac-static
< fac.1
''')
else:
    print("# no checkinstall+help2man, so we won't build a debian package")

if have_help2man:
    print('''
| help2man --no-info fac > fac.1
< fac''')
else:
    print("# no help2man, so we won't build the web page")

if have_sass:
    print('''
| sass -I. web/style.scss web/style.css
> web/style.css
C .sass-cache
''')
else:
    print("# no sass, so we won't build style.css")
