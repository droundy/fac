#!/usr/bin/python2

import os, hashlib

def make_name(i, n):
    path = '.'
    digits = '%d' % i
    while len(digits) < n:
        digits = '0'+digits
    for d in digits:
        path = path+'/number-'+d
    return path[2:]

def make_path(i, n):
    path = 'bench/temp'
    digits = '%d' % i
    while len(digits) < n:
        digits = '0'+digits
    for d in ['test-%d' % n]+['number-'+x for x in digits]:
        try:
            os.mkdir(path)
        except:
            pass
        path = path+'/'+d
    return path

allowed_chars = 'abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ'

def hashid(n):
    m = hashlib.sha1()
    m.update(str(n))
    name = ''
    for i in m.digest()[:16]:
        name += allowed_chars[ord(i) % len(allowed_chars)]
    return name

def hash_integer(n):
    m = hashlib.sha1()
    m.update(str(n))
    name = 0
    for i in m.digest()[:8]:
        name = name*256 + ord(i)
    return name

os.system('rm -rf bench/temp')

def create_bench(n):
    bilgef = open(make_path(0,n)[:-17]+'top.bilge', 'w')
    tupf = open(make_path(0,n)[:-17]+'Tupfile', 'w')
    makef = open(make_path(0,n)[:-17]+'Makefile', 'w')
    makef.write('all:')
    for i in range(10**n):
        makef.write(' %s.o' % make_name(i, n))
    makef.write('\n\n')
    for i in range(10**n):
        base = make_path(i, n)
        cname = make_name(i, n) + '.c'
        hname = make_name(i, n) + '.h'
        includes = ''
        funcs = ''
        for xx in [n+ii for ii in range(10)]:
            includes += '#include "%s.h"\n' % make_name(hash_integer(xx) % 10**n, n)
            funcs += '    %s();\n' % hashid(hash_integer(xx) % 10**n)
        makef.write("""
# %d
%s.o: %s.c
\tgcc -I. -O2 -c -o %s.o %s.c
""" % (i, make_name(i, n), make_name(i, n), make_name(i, n), make_name(i, n)))
        tupf.write("""
# %d
%s.c |> gcc -I. -O2 -c -o %%o %%f |> %s.o
""" % (i, make_name(i, n), make_name(i, n)))
        bilgef.write("""
# %d
| gcc -I. -O2 -c -o %s.o %s.c
> %s.o
""" % (i, make_name(i, n), make_name(i, n), make_name(i, n)))
        f = open(base+'.c', 'w')
        f.write('\n')
        f.write("""/* c file %s */

%s#include "%s"

#include <stdio.h>

void %s() {
    printf("Hello world %s!\\n");
%s}
""" % (cname, includes, hname, hashid(i), hashid(i), funcs))
        f.close()
        f = open(base+'.h', 'w')
        f.write("""/* header %s */
#ifndef %s_H
#define %s_H

void %s();

#endif
""" % (hname, hashid(i), hashid(i), hashid(i)))
        f.close()

create_bench(2)
