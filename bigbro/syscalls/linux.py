#!/usr/bin/python2

import re, os

import gentables

re_syscall = re.compile(r'#define __NR_(\S+)\s+([0-9]+)')
re_unused = re.compile(r'/\*\s+([0-9]+)\s+is\s+')

for postfix in ['_32', '_64']:
    if os.uname()[4] == 'x86_64':
        unistd_h = "syscalls/linux/unistd%s.h" % postfix
    else:
        unistd_h = "syscalls/linux/unistd_32.h"

    with open(unistd_h, 'r') as unistd_file:
        unistd = unistd_file.read()

    sysnames = []
    for line in unistd.split('\n'):
        m = re_syscall.match(line)
        if m != None:
            name, num = m.groups()
            #print 'got', num, name
            assert(int(num) == len(sysnames))
            sysnames.append(name)
        else:
            m = re_unused.match(line)
            if m != None:
                #print 'matched', line
                num = m.groups()[0]
                assert(int(num) == len(sysnames))
                sysnames.append('unused')

    gentables.tables(sysnames, postfix)
