#!/usr/bin/python2

import sys, re, os

if len(sys.argv) != 2:
    print "usage: python2 %s syscalls-file > syscalls.h" % sys.argv[0]
    sys.exit(1)

linux_dir = sys.argv[1]

re_syscall = re.compile(r'/\* (\d+) = (\S+)')

def bool_table(name, syscalls):
    print """
const int %s[] = {""" % (name)

    for i in range(maxnum):
        if sysnames[i] in syscalls:
            print '  1,'
        else:
            print '  0,'

    if sysnames[maxnum] in syscalls:
        print '  1'
    else:
        print '  0'
    print "};\n"


def argument_table(name, syscall_argument):
    print """
const int %s[] = {""" % (name)

    for i in range(maxnum):
        if sysnames[i] in syscall_argument:
            print '  %d,' % syscall_argument[sysnames[i]]
        else:
            print '  -1,'

    if sysnames[maxnum] in syscall_argument:
        print '  %d' % syscall_argument[sysnames[maxnum]]
    else:
        print '  -1'
    print "};\n"

with open(sys.argv[1], 'r') as syscalls_file:
    syscalls = syscalls_file.read()

sysnames = {}
sysnums = {}
maxnum = 0
for (num, syscall) in re_syscall.findall(syscalls):
    num = int(num)
    sysnames[num] = syscall
    sysnames[syscall] = num
    maxnum = max(maxnum, num)

print "\nconst char *syscalls[] = {"

for i in range(maxnum):
    if i not in sysnames:
        sysnames[i] = 'unused'
for i in range(maxnum):
    print '  "%s",' % sysnames[i]

print '  "%s"' % sysnames[maxnum]
print "};\n"

argument_table('fd_argument', { 'read': 0,
                                'write': 0,
                                'pread': 0,
                                'pwrite': 0,
                                'pread64': 0,
                                'pwrite64': 0,
                                'readv': 0,
                                'writev': 0,
                                'close': 0,
                                'fstat': 0,
                                'fstat64': 0,
                                'mmap': 4,
                                'sendfile': 1, # technically also 0!  :(
                                'fcntl': 0,
                                'flock': 0,
                                'fsync': 0,
                                'fdatasync': 0,
                                'ftruncate': 0,
                                'fchmod': 0,
                                'fchown': 0,
                                'fstatfs': 0,
                                'fadvise64': 0,
                                'fallocate': 0,
                                })


bool_table('fd_return', ['open',
                         'creat',
                         'openat',
                         'open_by_handle_at',
                         ])

argument_table('string_argument', { 'open': 0,
                                    'stat': 0,
                                    'lstat': 0,
                                    'stat64': 0,
                                    'lstat64': 0,
                                    'execve': 0,
                                    'access': 0,
                                    'rename': 1, # also 0
                                    })

bool_table('is_wait_or_exit', ['wait4',
                               'waitid',
                               'epoll_pwait',
                               'epoll_wait',
                               'futex',
                               'select',
                               'exit',
                               'exit_group'])

argument_table('write_fd', {'write': 0,
                            'writev': 0,
                            'pwrite': 0,
                            'pwrite64': 0,
                            'sendfile': 0,
                            'ftruncate': 0,
                            'fallocate': 0})

argument_table('write_string', {'rename': 1,
                                'truncate': 0})

argument_table('read_fd', {'read': 0,
                           'readv': 0,
                           'pread': 0,
                           'pread64': 0,
                           'fstat': 0,
                           'fstat64': 0,
                           'mmap': 0,
                           'sendfile': 1})

argument_table('read_string', {'rename': 1,
                               'stat': 0,
                               'lstat': 0,
                               'stat64': 0,
                               'lstat64': 0,
                               'execve': 0,
                               'truncate': 0})

argument_table('unlink_string', {'unlink': 0,
                                 'rmdir': 0,
                                 'rename': 0})

argument_table('unlinkat_string', {'unlinkat': 0,
                                   'renameat': 0})
argument_table('renameat_string', {'renameat': 2})

argument_table('readdir_fd', {'getdents': 0,
                              'getdents64': 0})
