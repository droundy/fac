#!/usr/bin/python3

import os

name='independent'

def open_and_gitadd(fname):
    f = open(fname, 'w')
    assert(not os.system('git add '+fname))
    return f

def create_bench(n):
    sconsf = open_and_gitadd('SConstruct')
    facf = open_and_gitadd('top.fac')
    open_and_gitadd('Tupfile.ini')
    sconsf.write("""
env = Environment()
""")
    for i in range(1,n+1):
        cname = 'hello_%d.c' % i
        with open_and_gitadd(cname) as f:
            f.write(r"""
#include <stdio.h>

int main() {
  printf("Hello %d\n");
  return 0;
}
""" % i)
        facf.write("""
| gcc -o hello_{i}.exe hello_{i}.c
> hello_{i}.exe
""".format(i=i))
        sconsf.write("""
env.Program('hello_%d.exe', 'hello_%d.c')
""" % (i, i))

verbs = ['building', 'rebuilding', 'modifying-c', 'doing-nothing']

def prepare():
    return {'rebuilding': 'rm -f *.exe',
            'modifying-c': 'echo >> hello_7.c'}

if __name__ == "__main__":
    import sys
    size = 10
    if len(sys.argv) > 1:
        size = int(sys.argv[1])
    create_bench(size)
    with open("modifying-c.sh", "w") as f:
        f.write(prepare()["modifying-c"])
