#!/usr/bin/python3
from __future__ import print_function

import glob, os, subprocess, platform, sys

# the following is needed for some tests to run under vagrant, since
# git complains about the default email setting.
if 'GIT_AUTHOR_EMAIL' not in os.environ:
    os.putenv("GIT_AUTHOR_EMAIL", 'Tester <test@example.com>')
    os.putenv("GIT_AUTHOR_NAME", 'Tester')
    os.putenv("GIT_COMMITTER_EMAIL", 'Tester <test@example.com>')
    os.putenv("GIT_COMMITTER_NAME", 'Tester')

# we always run with test coverage if gcovr is present!
have_gcovr = os.system('gcovr -h') == 0

def system(cmd):
    return subprocess.call(cmd, shell=True)

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'

# this ensures that when fac calls git it already has index prepared:
system('git status')

try:
    version = subprocess.check_output(['git', 'describe', '--dirty'])
except:
    version = subprocess.check_output(['git', 'rev-parse', 'HEAD'])
version = version[:-1]
tarname = 'fac-%s.tar.gz' % version

print('building on', platform.system())
if platform.system() == 'Windows':
    if system('./fac fac.exe -v'):
        print('Build failed!')
        exit(1)
else:
    if system('MINIMAL=1 ./fac --script build/%s.sh -i version-identifier.h -i README.md -i COPYING --tar %s fac'
              % (platform.system().lower(), tarname)):
        print('Build failed!')
        exit(1)

    if system('MINIMAL=1 ./fac --script build/%s.sh fac'
              % (platform.system().lower())):
        print('Build failed!')
        exit(1)
system('echo rm -rf bigbro >> build/%s.sh' % platform.system().lower())
system('chmod +x build/%s.sh' % platform.system().lower())

run_fac = './fac'
if have_gcovr:
    run_fac = 'COVERAGE=1 ./fac'
if system(run_fac):
    print('Build failed!')
    exit(1)
else:
    print('XXXXXXXXX built the fac we will test using', run_fac)

numpassed = 0
numfailed = 0
numskipped = 0

biggestname = max([len(f) for f in glob.glob('tests/*.sh') + glob.glob('tests/*.test')])

def write_script_name(n, num=0, tot=0):
    if tot > 0:
        n += ' (%d/%d)' % (num, tot)
    extralen = 8 # len(' (%d/%d)' % (tot,tot))
    sys.stdout.write(n+':')
    sys.stdout.flush()
    sys.stdout.write(' '*(biggestname+extralen+3-len(n)))

if system('cd bigbro && python3 run-tests.py'):
    write_script_name('ran all bigbro tests')
    print(bcolors.FAIL+'FAIL', bcolors.ENDC)
    numfailed += 1
else:
    write_script_name('ran all bigbro tests')
    print(bcolors.OKGREEN+'PASS', bcolors.ENDC)
    numpassed += 1

sh_tests = sorted(glob.glob('tests/*.sh'))
num_sh = len(sh_tests);
for i in range(num_sh):
    sh = sh_tests[i]
    write_script_name(sh, i, num_sh)
    cmdline = 'bash %s > %s.log 2>&1' % (sh, sh)
    exitval = system(cmdline)
    if exitval == 137:
        print(bcolors.OKBLUE+'SKIP', bcolors.ENDC)
        if '-v' in sys.argv:
            os.system('cat %s.log' % sh)
        numskipped += 1
    elif exitval:
        print(bcolors.FAIL+'FAIL', bcolors.ENDC)
        if '-v' in sys.argv:
            os.system('cat %s.log' % sh)
        numfailed += 1
    else:
        print(bcolors.OKGREEN+'PASS', bcolors.ENDC)
        numpassed += 1
for sh in sorted(glob.glob('tests/*.test')):
    if 'assertion-fails' in sh:
        continue
    write_script_name(sh)
    if system('%s > %s.log 2>&1' % (sh, sh)):
        print(bcolors.FAIL+'FAIL', bcolors.ENDC)
        if '-v' in sys.argv:
            os.system('cat %s.log' % sh)
        numfailed += 1
    else:
        print(bcolors.OKGREEN+'PASS', bcolors.ENDC)
        numpassed += 1

expectedfailures = 0
unexpectedpasses = 0

for sh in sorted(glob.glob('bugs/*.sh')):
    write_script_name(sh)
    if system('bash %s > %s.log 2>&1' % (sh, sh)):
        print(bcolors.OKGREEN+'fail', bcolors.ENDC)
        expectedfailures += 1
    else:
        print(bcolors.FAIL+'pass', bcolors.ENDC, sh)
        if '-v' in sys.argv:
            os.system('cat %s.log' % sh)
        unexpectedpasses += 1
for sh in sorted(glob.glob('bugs/*.test')):
    write_script_name(sh)
    if system('bash %s > %s.log 2>&1' % (sh, sh)):
        print(bcolors.OKGREEN+'fail', bcolors.ENDC)
        expectedfailures += 1
    else:
        print(bcolors.FAIL+'pass', bcolors.ENDC, sh)
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

if have_gcovr:
    os.system('rm -f test.*') # generated while testing compiler flags
    os.system('rm -f *-win.gc*') # generated while testing compiler flags
    os.system('rm -f bigbro/*.gc*') # not interested in bigbro coverage
    os.system('rm -f bigbro/*/*.gc*') # not interested in bigbro coverage
    os.system('rm -f tests/*.gc*') # not interested in test binaries
    assert not os.system('gcovr --gcov-exclude tests/ -k -r . --exclude-unreachable-branches --html --html-details -o web/coverage.html')
    assert not os.system('gcovr --gcov-exclude tests/ -r . --exclude-unreachable-branches')
else:
    print('not running gcovr')

print()
if numfailed:
    print(bcolors.FAIL+'Failed', str(numfailed)+'/'+str(numfailed+numpassed)+bcolors.ENDC)
else:
    print('All', pluralize(numpassed, 'test'), 'passed!')

if expectedfailures:
    print(pluralize(expectedfailures, 'expected failure'))
if numskipped:
    print(pluralize(numskipped, 'test'), 'skipped')

if unexpectedpasses:
    print(bcolors.FAIL+pluralize(unexpectedpasses, 'unexpected pass')+bcolors.ENDC)

if numfailed:
    exit(1)
