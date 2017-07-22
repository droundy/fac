#!/usr/bin/python3
from __future__ import print_function

import glob, os, subprocess, platform, sys

import build
build.elapsed_time()

# the following is needed for some tests to run under vagrant, since
# git complains about the default email setting.
if 'GIT_AUTHOR_EMAIL' not in os.environ:
    os.putenv("GIT_AUTHOR_EMAIL", 'Tester <test@example.com>')
    os.putenv("GIT_AUTHOR_NAME", 'Tester')
    os.putenv("GIT_COMMITTER_EMAIL", 'Tester <test@example.com>')
    os.putenv("GIT_COMMITTER_NAME", 'Tester')

# we always run with test coverage if gcovr is present!
have_gcovr = os.system('gcovr -h') == 0
have_cargo = os.system('cargo --version') == 0

def system(cmd):
    # print("running:", cmd)
    x = subprocess.call(cmd, shell=True)
    # assert(x == 0)
    return x

# this ensures that when fac calls git it already has index prepared:
system('git status')

try:
    version = subprocess.check_output(['git', 'describe', '--dirty'])
except:
    version = subprocess.check_output(['git', 'rev-parse', 'HEAD'])
version = version[:-1]
tarname = 'fac-%s.tar.gz' % version

if 'rust' not in sys.argv:
    print('building on', platform.system())
    if platform.system() == 'Windows':
        if system('./fac fac.exe -v'):
            print('Build failed!')
            exit(1)
    else:
        print(build.blue("creating build script and tarball"))
        if system('MINIMAL=1 ./fac --script build/%s.sh -i version-identifier.h -i README.md -i COPYING --tar %s fac'
                  % (platform.system().lower(), tarname)):
            print('Build failed!')
            exit(1)
        print(build.took('creating tarball'))

        print(build.blue("creating build script but not tarball"))
        if system('MINIMAL=1 ./fac --script build/%s.sh fac'
                  % (platform.system().lower())):
            print('Build failed!')
            exit(1)
        print(build.took('build script'))
    system('mv %s web/' % tarname)
    system('ln -sf %s web/fac.tar.gz' % tarname)
    system('echo rm -rf bigbro >> build/%s.sh' % platform.system().lower())
    system('chmod +x build/%s.sh' % platform.system().lower())

    print(build.blue("building with fac"))
    if system('./fac'):
        print('Build with fac failed!')
        exit(1)
    print(build.took('intial compiling'))

    if have_gcovr:
        os.system('rm -f *.gc* */*.gc*') # remove any preexisting coverage files
        if system('COVERAGE=1 ./fac fac'):
            print('Build with coverage failed!')
            exit(1)
        system('cp fac tests/fac-with-coverage')
        system('COVERAGE=1 tests/fac-with-coverage -c')
        system('rm -rf bigbro') # needed because -c doesn't remove cached files!  :(
        system('COVERAGE=1 tests/fac-with-coverage')
        system('rm tests/fac-with-coverage')
        print(build.took('rebuilding with fac and coverage'))
else:
    if system('./debug-fac debug-fac rust-fac'):
        print('Build with fac failed!')
        exit(1)
    print(build.took('building fac using rust'))


def shorten_name(n):
    if '/' in n:
        n = n.split('/')[1]
    if '.sh' in n:
        n = n[:-3]
    return n
def write_script_name(n, num=0, tot=0):
    n = shorten_name(n)
    if tot > 0:
        n += ' (%d/%d)' % (num+1, tot)
    extralen = 8 # len(' (%d/%d)' % (tot,tot))
    sys.stdout.write(n+':')
    sys.stdout.flush()
    sys.stdout.write(' '*(biggestname+extralen+3-len(n)))

biggestname = max([len(shorten_name(f))
                   for f in glob.glob('tests/*.sh') + glob.glob('tests/*.test')])

os.environ['FAC'] = os.getcwd()+"/fac"

numpassed = 0
numfailed = 0
numskipped = 0
expectedfailures = 0
unexpectedpasses = 0


failures = []

sh_tests = sorted(glob.glob('tests/*.sh'))
num_sh = len(sh_tests);
if 'rust' not in sys.argv:
    for i in range(num_sh):
        sh = sh_tests[i]
        with open(sh) as f:
            expect_failure = 'expect C failure' in f.read()
        write_script_name(sh, i, num_sh)
        cmdline = 'bash %s > %s.log 2>&1' % (sh, sh)
        exitval = system(cmdline)
        if expect_failure:
            if exitval:
                print(build.green('fail'), build.took())
                expectedfailures += 1
            else:
                print(build.red('pass'), sh, build.took())
                if '-v' in sys.argv:
                    os.system('cat %s.log' % sh)
                unexpectedpasses += 1
        elif exitval == 137:
            print(build.blue('SKIP'), build.took())
            if '-v' in sys.argv:
                os.system('cat %s.log' % sh)
            numskipped += 1
        elif exitval:
            print(build.FAIL, build.took())
            if '-v' in sys.argv:
                os.system('cat %s.log' % sh)
            numfailed += 1
            failures.append(shorten_name(sh))
        else:
            print(build.PASS, build.took())
            numpassed += 1
    for sh in sorted(glob.glob('tests/*.test')):
        if 'assertion-fails' in sh:
            continue
        write_script_name(sh)
        if system('%s > %s.log 2>&1' % (sh, sh)):
            print(build.FAIL, build.took())
            if '-v' in sys.argv:
                os.system('cat %s.log' % sh)
            numfailed += 1
            failures.append(shorten_name(sh))
        else:
            print(build.PASS, build.took())
            numpassed += 1

    for sh in sorted(glob.glob('bugs/*.sh')):
        write_script_name(sh)
        if system('bash %s > %s.log 2>&1' % (sh, sh)):
            print(build.green('fail'), build.took())
            expectedfailures += 1
        else:
            print(build.red('pass'), sh, build.took())
            if '-v' in sys.argv:
                os.system('cat %s.log' % sh)
            unexpectedpasses += 1
    for sh in sorted(glob.glob('bugs/*.test')):
        write_script_name(sh)
        if system('bash %s > %s.log 2>&1' % (sh, sh)):
            print(build.green('fail'), build.took())
            expectedfailures += 1
        else:
            print(build.red('pass'), build.took())
            if '-v' in sys.argv:
                os.system('cat %s.log' % sh)
            unexpectedpasses += 1

def pluralize(num, noun):
    if num == 1:
        return str(num)+' '+noun
    else:
        if (noun[-1] == 's'):
            return str(num)+' '+noun+'es'
        return str(num)+' '+noun+'s'

os.environ['FAC'] = os.getcwd()+"/debug-fac"

rust_numpassed = 0
rust_numfailed = 0
rust_numskipped = 0

rust_numexpectedfailed = 0
rust_numunexpectedpassed = 0

if have_cargo:
    for i in range(num_sh):
        sh = sh_tests[i]
        with open(sh) as f:
            expect_failure = 'expect rust failure' in f.read()
        write_script_name(sh, i, num_sh)
        cmdline = 'bash %s > %s.rust.log 2>&1' % (sh, sh)
        exitval = system(cmdline)
        if expect_failure:
            if exitval == 137:
                print(build.blue('SKIP'), "(rust)", build.took())
                if '-v' in sys.argv:
                    os.system('cat %s.rust.log' % sh)
                rust_numskipped += 1
            elif exitval:
                print(build.warn('fail'), "(rust)", build.took())
                if '-v' in sys.argv:
                    os.system('cat %s.rust.log' % sh)
                rust_numexpectedfailed += 1
            else:
                print(build.red('pass'), "(rust)", build.took())
                rust_numunexpectedpassed += 1
        else:
            if exitval == 137:
                print(build.blue('SKIP'), "(rust)", build.took())
                if '-v' in sys.argv:
                    os.system('cat %s.rust.log' % sh)
                rust_numskipped += 1
            elif exitval:
                print(build.FAIL, "(rust)", build.took())
                if '-v' in sys.argv:
                    os.system('cat %s.rust.log' % sh)
                rust_numfailed += 1
            else:
                print(build.PASS, "(rust)", build.took())
                rust_numpassed += 1

if have_gcovr:
    os.system('rm -f test.*') # generated while testing compiler flags
    os.system('rm -f *-win.gc* *-static.gc* *-afl.gc*') # generated for other builds than "standard"
    os.system('rm -f cc*.gc*') # not sure where these come from!
    os.system('rm -f bigbro/*.gc*') # not interested in bigbro coverage
    os.system('rm -f bigbro/*/*.gc*') # not interested in bigbro coverage
    os.system('rm -f tests/*.gc*') # not interested in test binaries
    assert not os.system('gcovr --exclude-directories tests -k -r . --exclude-unreachable-branches --html --html-details -o web/coverage.html')
    assert not os.system('gcovr --exclude-directories tests -k -r . --exclude-unreachable-branches')
else:
    print('not running gcovr')

print()
if have_cargo:
    if rust_numfailed:
        print(build.red('Rust failed ' + str(rust_numfailed)+'/'+str(rust_numfailed+rust_numpassed)))
    else:
        print('All', pluralize(rust_numpassed, 'test'), 'passed using rust!')

    if rust_numunexpectedpassed:
        print(build.red('Rust unexpectedly passed ' + str(rust_numunexpectedpassed)
                        +'/'+str(rust_numunexpectedpassed+rust_numexpectedfailed)
                        +' expected failures'))
    else:
        print('All', pluralize(rust_numexpectedfailed, 'test'), 'exected failures failed using rust!')
else:
    rust_numfailed = 0

if numfailed:
    for f in failures:
        print(build.red('    FAILED '+f))
    print(build.red('Failed ' + str(numfailed)+'/'+str(numfailed+numpassed)))
else:
    print('All', pluralize(numpassed, 'test'), 'passed!')

if expectedfailures:
    print(pluralize(expectedfailures, 'expected failure'))
if numskipped:
    print(pluralize(numskipped, 'test'), 'skipped')

if unexpectedpasses:
    print(build.red(pluralize(unexpectedpasses, 'unexpected pass')))

if numfailed or rust_numfailed:
    exit(1)
