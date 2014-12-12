#!/usr/bin/python2

import sys, re, os

if len(sys.argv) != 2:
    print "usage: python %s /path/to/linux-headers > syscalls.h" % sys.argv[0]
    sys.exit(1)

linux_dir = sys.argv[1]

re_syscall = re.compile(r'define __NR_(\S+)\s+([0-9]+)')

if os.uname()[4] == 'x86_64':
    unistd_h = "/arch/x86/include/asm/unistd_64.h"
else:
    unistd_h = "/arch/x86/include/asm/unistd_32.h"

with open(linux_dir+unistd_h, 'r') as unistd_file:
    unistd = unistd_file.read()

sysnames = {}
sysnums = {}
maxnum = 0
for (syscall, num) in re_syscall.findall(unistd):
    num = int(num)
    sysnames[num] = syscall
    sysnames[syscall] = num
    maxnum = max(maxnum, num)

print """
const char *syscalls[] = {"""

for i in range(maxnum):
    print '  "%s",' % sysnames[i]

print '  "%s"' % sysnames[maxnum]
print "};\n"

fd_argument = {
    'read': 0,
    'write': 0,
    'close': 0,
    'fstat': 0,
    'mmap': 4,
}

print """
const int fd_argument[] = {"""

for i in range(maxnum):
    if sysnames[i] in fd_argument:
        print '  %d,' % fd_argument[sysnames[i]]
    else:
        print '  -1,'

if sysnames[maxnum] in fd_argument:
    print '  %d' % fd_argument[sysnames[maxnum]]
else:
    print '  -1'
print "};\n"
