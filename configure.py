#!/usr/bin/python3

from __future__ import print_function
import string, os, sys, platform, subprocess

myplatform = sys.platform
if myplatform == 'linux2':
    myplatform = 'linux'

def is_in_path(program):
    """ Does the program exist in the PATH? """
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)
    fpath, fname = os.path.split(program)
    if fpath:
        return is_exe(program)
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return True
    return False

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

def cargo_cmd(cmd, inps, outs):
        print('\n| {}'.format(cmd))
        for i in inps:
            print("< {}".format(i))
        for o in outs:
            print("> {}".format(o))
        if 'cargo-test-output.log' not in outs:
            print('c .log')
        print('''c ~
c #
C .nfs
c ~
c .fac
c .tum
c .pyc
c .o
c fac.exe
c __pycache__
c .gcda
c .gcno
c .gcov
c src/version.rs
c Cargo.lock
c fac
c fac-afl
c -pak
c .deb
c .1
C doc-pak
C bench
C tests
C bugs
C web
C target
C bigbro
''')

if is_in_path('cargo'):
    cargo_cmd("cargo build --features strict && mv target/debug/fac debug-fac", [],
              ["debug-fac"])
    # cargo_cmd("cargo test --features strict > cargo-test-output.log",
    #           [], ['cargo-test-output.log'])
    cargo_cmd("cargo doc --no-deps && cp -a target/doc web/", [],
              ["web/doc/fac/index.html"])
    cargo_cmd("cargo build --release && mv target/release/fac fac", [],
              ["fac"])
    print("""
# make copies of the executables, so that if cargo fails we will still
# have an old version of the executable, since cargo deletes output on
# failure.
| cp debug-fac backup-debug-fac
< debug-fac
> backup-debug-fac

| cp fac backup-fac
< fac
> backup-fac""")
else:
    print('# no cargo, so cannot build using rust')

try:
    targets = subprocess.check_output('rustup show', shell=True)
    if b'x86_64-pc-windows-gnu' in targets:
        cargo_cmd("cargo build --features strict --target x86_64-pc-windows-gnu", [], [])
except:
    print('# no rust for windows')
