#!/usr/bin/python2

import glob, os, subprocess, platform, sys

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

if system('MINIMAL=1 ./fac --makefile Makefile.%s --script build-%s.sh fac'
          % (platform.system().lower(), platform.system().lower())):
    print 'Build failed!'
    exit(1)

scriptf = open('build-%s.sh' % platform.system().lower(), 'a')
scriptf.write('''
rm -rf bigbro
''')
scriptf.close()

if system('./fac'):
    print 'Build failed!'
    exit(1)

numpassed = 0
numfailed = 0

biggestname = max([len(f) for f in glob.glob('tests/*.sh') + glob.glob('tests/*.test')])

def write_script_name(n):
    sys.stdout.write(n+':')
    sys.stdout.flush()
    sys.stdout.write(' '*(biggestname+3-len(n)))

write_script_name('running all bigbro tests')
if system('cd bigbro && python3 run-tests.py > /dev/null'):
    print bcolors.FAIL+'FAIL', bcolors.ENDC
    numfailed += 1
else:
    print bcolors.OKGREEN+'PASS', bcolors.ENDC
    numpassed += 1

for sh in sorted(glob.glob('tests/*.sh')):
    write_script_name(sh)
    cmdline = 'bash %s > %s.log 2>&1' % (sh, sh)
    if system(cmdline):
        print bcolors.FAIL+'FAIL', bcolors.ENDC
        numfailed += 1
    else:
        print bcolors.OKGREEN+'PASS', bcolors.ENDC
        numpassed += 1
for sh in sorted(glob.glob('tests/*.test')):
    if 'assertion-fails' in sh:
        continue
    write_script_name(sh)
    if system('%s > %s.log 2>&1' % (sh, sh)):
        print bcolors.FAIL+'FAIL', bcolors.ENDC
        numfailed += 1
    else:
        print bcolors.OKGREEN+'PASS', bcolors.ENDC
        numpassed += 1

expectedfailures = 0
unexpectedpasses = 0

for sh in sorted(glob.glob('bugs/*.sh')):
    write_script_name(sh)
    if system('bash %s > %s.log 2>&1' % (sh, sh)):
        print bcolors.OKGREEN+'fail', bcolors.ENDC
        expectedfailures += 1
    else:
        print bcolors.FAIL+'pass', bcolors.ENDC, sh
        unexpectedpasses += 1
for sh in sorted(glob.glob('bugs/*.test')):
    write_script_name(sh)
    if system('bash %s > %s.log 2>&1' % (sh, sh)):
        print bcolors.OKGREEN+'fail', bcolors.ENDC
        expectedfailures += 1
    else:
        print bcolors.FAIL+'pass', bcolors.ENDC, sh
        unexpectedpasses += 1

def pluralize(num, noun):
    if num == 1:
        return str(num)+' '+noun
    else:
        if (noun[-1] == 's'):
            return str(num)+' '+noun+'es'
        return str(num)+' '+noun+'s'

print
if numfailed:
    print bcolors.FAIL+'Failed', str(numfailed)+'/'+str(numfailed+numpassed)+bcolors.ENDC
else:
    print 'All', pluralize(numpassed, 'test'), 'passed!'

if expectedfailures:
    print pluralize(expectedfailures, 'expected failure')

if unexpectedpasses:
    print bcolors.FAIL+pluralize(unexpectedpasses, 'unexpected pass')+bcolors.ENDC

if numfailed:
    exit(1)
