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

system('./fac -c')
system('cargo build')
if system('target/debug/fac debug-fac fac'):
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

for i in range(num_sh):
    sh = sh_tests[i]
    with open(sh) as f:
        expect_failure = 'expect rust failure' in f.read()
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

debug_numpassed = 0
debug_numfailed = 0
debug_numskipped = 0

debug_numexpectedfailed = 0
debug_numunexpectedpassed = 0

for i in range(num_sh):
    sh = sh_tests[i]
    with open(sh) as f:
        expect_failure = 'expect rust failure' in f.read()
    write_script_name(sh, i, num_sh)
    cmdline = 'bash %s > %s.debug.log 2>&1' % (sh, sh)
    exitval = system(cmdline)
    if expect_failure:
        if exitval == 137:
            print(build.blue('SKIP'), "(debug)", build.took())
            if '-v' in sys.argv:
                os.system('cat %s.debug.log' % sh)
            debug_numskipped += 1
        elif exitval:
            print(build.warn('fail'), "(debug)", build.took())
            if '-v' in sys.argv:
                os.system('cat %s.debug.log' % sh)
            debug_numexpectedfailed += 1
        else:
            print(build.red('pass'), "(debug)", build.took())
            debug_numunexpectedpassed += 1
    else:
        if exitval == 137:
            print(build.blue('SKIP'), "(debug)", build.took())
            if '-v' in sys.argv:
                os.system('cat %s.debug.log' % sh)
            debug_numskipped += 1
        elif exitval:
            print(build.FAIL, "(debug)", build.took())
            if '-v' in sys.argv:
                os.system('cat %s.debug.log' % sh)
            debug_numfailed += 1
        else:
            print(build.PASS, "(debug)", build.took())
            debug_numpassed += 1

print()
if debug_numfailed:
    print(build.red('Debug failed ' + str(debug_numfailed)+'/'+str(debug_numfailed+debug_numpassed)))
else:
    print('All', pluralize(debug_numpassed, 'test'), 'passed using debug!')

if debug_numunexpectedpassed:
    print(build.red('Debug unexpectedly passed ' + str(debug_numunexpectedpassed)
                    +'/'+str(debug_numunexpectedpassed+debug_numexpectedfailed)
                    +' expected failures'))
else:
    print('All', pluralize(debug_numexpectedfailed, 'test'), 'exected failures failed using debug!')

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

if numfailed or debug_numfailed:
    exit(1)
