#!/usr/bin/python3

import os, hashlib, time, numpy, sys, datetime

name='dependent-chain'

allowed_chars = 'abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ'

def hashid(n):
    m = hashlib.sha1()
    m.update(str(n).encode('utf-8'))
    name = ''
    for i in m.digest()[:24]:
        name += allowed_chars[i % len(allowed_chars)]
    return name+('_%d' % n)

def create_bench(n):
    sconsf = open('SConstruct', 'w')
    bilgef = open('top.bilge', 'w')
    tupf = open('Tupfile', 'w')
    open('Tupfile.ini', 'w')
    sconsf.write("""
env = Environment(CPPPATH=['.'])
""")
    bilgef.write("""
| gcc -o final.exe final.c
> final.exe
< final.c
< %s-generated.h

""" % hashid(n))
    f = open('final.c', 'w')
    f.write("""
#include "%s-generated.h"
#include <stdio.h>

void main() {
  printf("%%s\\n", %s);
}
""" % (hashid(n), hashid(n)))
    f.close()
    f = open('%s-generated.h' % hashid(0), 'w')
    f.write("""
const char *%s = "Hello world";

""" % hashid(0))
    f.close()
    for i in range(1,n+1):
        cname = hashid(i) + '.c'
        hname = hashid(i) + '-generated.h'
        f = open('%s.c' % hashid(i), 'w')
        f.write("""
#include <stdio.h>

int main() {
  printf("const char *%s = \\"Hello world\\";\\n");
  return 0;
}

""" % hashid(i))
        f.close()
        tupf.write("""
# %d
: %s.c | %s-generated.h |> gcc -o %%o %%f |> %s.exe

: %s.exe |> ./%%f > %%o |> %s-generated.h
""" % (i, hashid(i), hashid(i-1), hashid(i),
       hashid(i), hashid(i)))
        bilgef.write("""
# %d
| gcc -o %s.exe %s.c
> %s.exe
< %s-generated.h

| ./%s.exe > %s-generated.h
> %s-generated.h
< %s.exe
""" % (i, hashid(i), hashid(i), hashid(i), hashid(i-1),
       hashid(i), hashid(i), hashid(i), hashid(i)))
        sconsf.write("""
env.Program('%s.exe', '%s.c')
env.Command('%s-generated.h', '%s.exe', './$SOURCE > $TARGET')
""" % (hashid(i), hashid(i), hashid(i), hashid(i)))

verbs = ['building', 'rebuilding', 'touching-header', 'touching-c', 'doing-nothing']

def prepare():
    return {'rebuilding': 'sleep 1 && find . -name "*.c" -exec touch \{\} \;',
            'touching-header': 'echo >> %s-generated.h' % hashid(0),
            'touching-c': 'echo >> final.c'}
