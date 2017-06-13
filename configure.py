#!/usr/bin/python3

from __future__ import print_function
import string, os, sys, platform, subprocess

myplatform = sys.platform
if myplatform == 'linux2':
    myplatform = 'linux'

def can_run(cmd):
    print('# trying', cmd, file=sys.stdout)
    try:
        subprocess.check_output(cmd, shell=True)
        return True
    except Exception as e:
        print("# error: ", e)
        return False

have_sass = can_run('sass -h')
have_help2man = can_run('help2man --help')
have_checkinstall = can_run('checkinstall --version')

os.system('rm -rf testing-flags')
os.mkdir('testing-flags');
with open('testing-flags/test.c', 'w') as f:
    f.write("""
#define _XOPEN_SOURCE 500
#include <ftw.h> /* If we can't use ftw.h, then we've got a problem (e.g. old mingw) */

int main() {
  return 0;
}
""")

optional_flags = ['-Wall', '-Werror', '-g', '-O2', '-flto']
optional_linkflags = ['-lprofiler', '-flto']

# To enable coverage testing define the environment variable $COVERAGE
if os.getenv('COVERAGE') != None:
    optional_flags += ['--coverage', "-DCOVERAGE"]
    optional_linkflags += ['--coverage']

possible_flags = ['-std=c11', '-std=c99']
possible_linkflags = ['-lpthread', '-lm']

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
                            'linkflags': [os.getenv('LDFLAGS', ''), '-static'],
                            'os': platform.system().lower(),
                            'arch': platform.machine()},
                '-afl': {'cc': 'afl-gcc',
                            'flags': [os.getenv('CFLAGS', '') + ' -Ibigbro'],
                            'linkflags': [os.getenv('LDFLAGS', ''), '-static'],
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
    return can_run('%s %s -c -o testing-flags/test.o testing-flags/test.c' % (cc, ' '.join(flags)))
def link_works(flags):
    return can_run('%s -o testing-flags/test testing-flags/test.c %s' % (cc, ' '.join(flags)))

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

    sources = ['main', 'fac', 'files', 'targets', 'clean-all', 'build', 'git', 'environ',
               'mkdir', 'arguments']

    libsources = ['listset', 'iterablehash', 'sha1']

    for s in sources:
        print('| %s %s -o %s%s.o -c %s.c' % (cc, ' '.join(flags), s, variant, s))
        print('> %s%s.o' % (s, variant))
        if '-flto' in flags and s == 'main':
            print('c %s%s.gcno' % (s, variant))
        if s == 'fac':
            print('< version-identifier.h')
        print()

    for s in libsources:
        print('| cd lib && %s %s -o %s%s.o -c %s.c' % (cc, ' '.join(flags), s, variant, s))
        print('> lib/%s%s.o' % (s, variant))
        print()

    ctests = ['listset', 'spinner', 'iterable_hash_test', 'assertion-fails']

    if variant != '-win':
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
        print('c .gcno')
        print()

    if variant != '-win':
        build_fac()
    else:
        print('| %s -o fac.exe -static %s' %
              (cc,
               ' '.join(['%s%s.o' % (s, variant) for s in sources]
                        + ['bigbro/libbigbro-windows.a']
                        + ['lib/%s%s.o' % (s, variant) for s in libsources]
                        + linkflags)))
        print('< bigbro/libbigbro-windows.a')
        for s in sources:
            print('< %s%s.o' % (s, variant))
        for s in libsources:
            print('< lib/%s%s.o' % (s, variant))
        print('> fac.exe')
        print()

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
| help2man --no-info ./fac > fac.1
< fac''')
else:
    print("# no help2man, so we won't build the man page")

if have_sass:
    print('''
| sass -I. web/style.scss web/style.css
> web/style.css
C .sass-cache
''')
else:
    print("# no sass, so we won't build style.css")

if can_run('cargo help'):
    print('''
# cargo is annoying in that it calls stat on every file in the repository, making
# it near impossible to discern its inputs.  So we have to list everything as
# "cached" content.  :(
| cargo build --features strict && cargo build --release && cargo doc --no-deps && cargo test --features strict
< version-identifier.h
> target/debug/fac
C bench
c ~
c #
C .nfs
c .tum
c .gcno
c .gcda
c .gcov
C tests
C bugs
C web
c .pyc
c __init__
c -fac
C doc-pak
c -pak

| cp -a target/doc web/
> web/doc/fac/index.html
< target/doc/fac/index.html

# make copies of the executables, so that if cargo fails we will still
# have an old version of the executable, since cargo deletes output on
# failure.
| cp target/debug/fac debug-fac~ && mv debug-fac~ debug-fac
< target/debug/fac
> debug-fac

| cp target/release/fac rust-fac~ && mv rust-fac~ rust-fac
< target/release/fac
> rust-fac
''')
else:
    print('# no cargo, so cannot build using rust')

try:
    targets = subprocess.check_output('rustup show', shell=True)
    if b'x86_64-pc-windows-gnu' in targets:
        print('''
| cargo build --features strict --target x86_64-pc-windows-gnu
< target/debug/fac
< src/version.rs
C bench
C target/debug/build
c -lock
c ~
c #
C .nfs
c .tum
c .gcno
c .gcda
c .gcov
C tests
C bugs
C web
c .pyc
c __init__
c -fac
C doc-pak
c -pak
''')
except:
    print('# no rust for windows')
