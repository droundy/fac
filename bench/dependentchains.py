#!/usr/bin/python3

import os, hashlib, time, sys, datetime

name='dependent-chain'

allowed_chars = 'abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ'

def hashid(n):
    m = hashlib.sha1()
    m.update(str(n).encode('utf-8'))
    name = ''
    for i in m.digest()[:24]:
        name += allowed_chars[i % len(allowed_chars)]
    return name+('_%d' % n)

def open_and_gitadd(fname):
    f = open(fname, 'w')
    assert(not os.system('git add '+fname))
    return f

def create_bench(n):
    sconsf = open_and_gitadd('SConstruct')
    facf = open_and_gitadd('top.fac')
    open_and_gitadd('Tupfile.ini')
    sconsf.write("""
env = Environment(CPPPATH=['.'])
""")
    facf.write("""
| gcc -o final.exe final.c
> final.exe
< final.c
< %s-generated.h

""" % hashid(n))
    f = open_and_gitadd('final.c')
    f.write("""
#include "%s-generated.h"
#include <stdio.h>

void main() {
  printf("%%s\\n", %s);
}
""" % (hashid(n), hashid(n)))
    f.close()
    f = open_and_gitadd('%s-generated.h' % hashid(0))
    f.write("""
const char *%s = "Hello world";

""" % hashid(0))
    f.close()
    for i in range(1,n+1):
        cname = hashid(i) + '.c'
        hname = hashid(i) + '-generated.h'
        f = open_and_gitadd('%s.c' % hashid(i))
        f.write("""
#include <stdio.h>

int main() {
  printf("const char *%s = \\"Hello world\\";\\n");
  return 0;
}

""" % hashid(i))
        f.close()
        facf.write("""
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

verbs = ['building', 'rebuilding', 'modifying-header', 'modifying-c', 'doing-nothing']

def prepare():
    return {'rebuilding': 'rm -f *.exe',
            'modifying-header': 'echo >> %s-generated.h' % hashid(0),
            'modifying-c': 'echo >> final.c'}

if __name__ == "__main__":
    import sys
    size = 10
    if len(sys.argv) > 1:
        size = int(sys.argv[1])
    create_bench(size)
    with open("modifying-header.sh", "w") as f:
        f.write(prepare()["modifying-header"])
