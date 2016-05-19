#!/usr/bin/python3

import re, os

def bool_table(name, syscalls, sysnames, postfix):
    print("""
static const int %s%s[] = {""" % (name, postfix))

    successive_borings = ''
    for i in range(len(sysnames)-1):
        if sysnames[i] in syscalls:
            if len(successive_borings) > 0:
                print(' %s\n  1, /* %s */' % (successive_borings,
                                              sysnames[i]))
            else:
                print('  1, /* %s */' % sysnames[i])
            successive_borings = ''
        else:
            if len(successive_borings)/(1+successive_borings.count('\n')) > 75:
                successive_borings += '\n  0,'
            else:
                successive_borings += ' 0,'
    print(successive_borings)
    if sysnames[-1] in syscalls:
        print('  1')
    else:
        print('  0')
    print("};\n")


def argument_table(name, syscall_argument, sysnames, postfix):
    print("""
static const int %s%s[] = {""" % (name, postfix))

    successive_borings = ''
    for i in range(len(sysnames)-1):
        if sysnames[i] in syscall_argument:
            if len(successive_borings) > 0:
                print(' %s\n  %d, /* %s */' % (successive_borings,
                                               syscall_argument[sysnames[i]],
                                               sysnames[i]))
            else:
                print('  %d, /* %s */' % (syscall_argument[sysnames[i]],
                                          sysnames[i]))
            successive_borings = ''
        else:
            if len(successive_borings)/(1+successive_borings.count('\n')) > 75:
                successive_borings += '\n  -1,'
            else:
                successive_borings += ' -1,'
    print(successive_borings)
    if sysnames[-1] in syscall_argument:
        print('  %d' % syscall_argument[sysnames[-1]])
    else:
        print('  -1')
    print("};\n")

def tables(sysnames, postfix):
    maxnum = len(sysnames)-1
    print("""
static const char *syscalls%s[] = {""" % postfix)
    for i in range(len(sysnames)-1):
        print('  /*%4d */ "%s",' % (i, sysnames[i]))
    print('  "%s"' % sysnames[-1])
    print("};\n")

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
                                    },
                   sysnames, postfix)


    bool_table('fd_return', ['open',
                             'creat',
                             'openat',
                             'open_by_handle_at',
                             ],
               sysnames, postfix)

    argument_table('string_argument', { 'open': 0,
                                        'stat': 0,
                                        'lstat': 0,
                                        'stat64': 0,
                                        'lstat64': 0,
                                        'execve': 0,
                                        'access': 0,
                                        'rename': 1, # also 0
                                        },
                   sysnames, postfix)

    argument_table('write_fd', {'write': 0,
                                'writev': 0,
                                'pwrite': 0,
                                'pwrite64': 0,
                                'sendfile': 0,
                                'ftruncate': 0,
                                'fallocate': 0},
                   sysnames, postfix)

    argument_table('write_string', {'rename': 1,
                                    'truncate': 0},
                   sysnames, postfix)

    argument_table('read_fd', {'read': 0,
                               'readv': 0,
                               'pread': 0,
                               'pread64': 0,
                               'fstat': 0,
                               'fstat64': 0,
                               'mmap': 0,
                               'sendfile': 1},
                   sysnames, postfix)

    argument_table('read_string', {'rename': 1,
                                   'stat': 0,
                                   'lstat': 0,
                                   'stat64': 0,
                                   'lstat64': 0,
                                   'execve': 0,
                                   'truncate': 0},
                   sysnames, postfix)

    argument_table('unlink_string', {'unlink': 0,
                                     'rmdir': 0,
                                     'rename': 0},
                   sysnames, postfix)

    argument_table('unlinkat_string', {'unlinkat': 0,
                                       'renameat': 0},
                   sysnames, postfix)
    argument_table('renameat_string', {'renameat': 2},
                   sysnames, postfix)

    argument_table('readdir_fd', {'getdents': 0,
                                  'getdents64': 0},
                   sysnames, postfix)
