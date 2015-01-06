#!/usr/bin/python2

import glob, os

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'

if os.system('./bilge'):
    print 'Build failed!'
    exit(1)

numpassed = 0
numfailed = 0

for sh in glob.glob('tests/*.sh'):
    if os.system('bash %s > %s.log 2>&1' % (sh, sh)):
        print bcolors.FAIL+'FAIL:', bcolors.ENDC+sh
        numfailed += 1
    else:
        print bcolors.OKGREEN+'PASS:', bcolors.ENDC+sh
        numpassed += 1
for sh in glob.glob('tests/*.test'):
    if os.system('%s > %s.log 2>&1' % (sh, sh)):
        print bcolors.FAIL+'FAIL:', bcolors.ENDC+sh
        numfailed += 1
    else:
        print bcolors.OKGREEN+'PASS:', bcolors.ENDC+sh
        numpassed += 1

expectedfailures = 0
unexpectedpasses = 0

for sh in glob.glob('bugs/*.sh'):
    if os.system('bash %s > %s.log 2>&1' % (sh, sh)):
        print bcolors.OKGREEN+'fail:', bcolors.ENDC+sh
        expectedfailures += 1
    else:
        print bcolors.FAIL+'pass:', bcolors.ENDC, sh
        unexpectedpasses += 1
for sh in glob.glob('bugs/*.test'):
    if os.system('bash %s > %s.log 2>&1' % (sh, sh)):
        print bcolors.OKGREEN+'fail:', bcolors.ENDC+sh
        expectedfailures += 1
    else:
        print bcolors.FAIL+'pass:', bcolors.ENDC, sh
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
