#!/usr/bin/python2

import glob, os

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'

os.system('./bilge')

numpassed = 0
numfailed = 0

for sh in glob.glob('tests/*.sh'):
    if os.system('bash %s > %s.log 2>&1' % (sh, sh[:-3])):
        print sh, ':', bcolors.FAIL, 'FAILED', bcolors.ENDC
        numfailed += 1
    else:
        print sh, ':', bcolors.OKGREEN, 'PASSED', bcolors.ENDC
        numpassed += 1

expectedfailures = 0
unexpectedpasses = 0

for sh in glob.glob('bugs/*.sh'):
    if os.system('bash %s > %s.log 2>&1' % (sh, sh[:-3])):
        print sh, ':', bcolors.OKGREEN, 'expected failure', bcolors.ENDC
        expectedfailures += 1
    else:
        print sh, ':', bcolors.FAIL, 'unexpected pass', bcolors.ENDC
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
    print 'Failed', str(numfailed)+'/'+str(numfailed+numpassed)
else:
    print 'All', pluralize(numpassed, 'test'), 'passed!'

if expectedfailures:
    print pluralize(expectedfailures, 'expected failure')

if unexpectedpasses:
    print pluralize(unexpectedpasses, 'unexpected pass')
