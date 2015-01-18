#!/usr/bin/python2

import os, hashlib, time, numpy
import matplotlib.pyplot as plt

def make_basedir(n):
    return 'bench/temp-dep/dep-%d/' % n

allowed_chars = 'abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ'

def hashid(n):
    m = hashlib.sha1()
    m.update(str(n))
    name = ''
    for i in m.digest()[:24]:
        name += allowed_chars[ord(i) % len(allowed_chars)]
    return name+('_%d' % n)

def hash_integer(n):
    m = hashlib.sha1()
    m.update(str(n))
    name = 0
    for i in m.digest()[:8]:
        name = name*256 + ord(i)
    return name

os.system('rm -rf bench/temp-dep')

def create_bench(n):
    try:
        os.mkdir('bench/temp-dep')
    except:
        pass
    os.mkdir(make_basedir(n))
    sconsf = open(make_basedir(n)+'SConstruct', 'w')
    bilgef = open(make_basedir(n)+'top.bilge', 'w')
    tupf = open(make_basedir(n)+'Tupfile', 'w')
    makef = open(make_basedir(n)+'Makefile', 'w')
    open(make_basedir(n)+'Tupfile.ini', 'w')
    sconsf.write("""
env = Environment(CPPPATH=['.'])
""")
    makef.write("""
final.exe: final.c %s-generated.h
\tgcc -o final.exe final.c

clean:
\tmv %s-generated.h backup && rm -f *.o *.exe *-generated.h && mv backup %s-generated.h

""" % (hashid(n), hashid(0), hashid(0)))
    bilgef.write("""
| gcc -o final.exe final.c
> final.exe
< final.c
< %s-generated.h

""" % hashid(n))
    f = open('bench/temp-dep/dep-%d/final.c' % n, 'w')
    f.write("""
#include "%s-generated.h"
#include <stdio.h>

void main() {
  printf("%%s\\n", %s);
}
""" % (hashid(n), hashid(n)))
    f.close()
    f = open('bench/temp-dep/dep-%d/%s-generated.h' % (n, hashid(0)), 'w')
    f.write("""
const char *%s = "Hello world";

""" % hashid(0))
    f.close()
    for i in range(1,n+1):
        base = 'bench/temp-dep/dep-%d/' % n + hashid(i)
        cname = hashid(i) + '.c'
        hname = hashid(i) + '-generated.h'
        f = open('bench/temp-dep/dep-%d/%s.c' % (n, hashid(i)), 'w')
        f.write("""
#include <stdio.h>

int main() {
  printf("const char *%s = \\"Hello world\\";\\n");
  return 0;
}

""" % hashid(i))
        f.close()
        makef.write("""
# %d
%s.exe: %s.c %s-generated.h
\tgcc -o %s.exe %s.c

%s-generated.h: %s.exe
\t./%s.exe > %s-generated.h
""" % (i, hashid(i), hashid(i), hashid(i-1), hashid(i), hashid(i),
       hashid(i), hashid(i), hashid(i), hashid(i)))
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

the_time = {}

def time_command(nnn, builder):
    global the_time
    if not builder in the_time:
        the_time[builder] = {}

    cmd = 'cd %s && %s' % (make_basedir(nnn), builder)
    cmd = 'cd %s && %s > output 2>&1' % (make_basedir(nnn), builder)

    cleanit = 'cd %s && make clean > clean_output 2>&1 && rm -rf .tup && rm -f *.done && rm -f .sco* || true' % make_basedir(nnn)

    repeats = 1
    verb = 'building'
    if not verb in the_time[builder]:
        the_time[builder][verb] = {}
    for i in range(repeats):
        os.system(cleanit)
        start = time.time()
        #print(cmd)
        assert(not os.system(cmd))
        stop = time.time()
        print '%s %s took %g seconds.' % (verb, builder, stop - start)
        the_time[builder][verb][nnn] = stop - start

    rebuild = 'cd %s && touch *.c */*.c */*/*.c */*/*/*.c */*/*/*/*.c > touch_output 2>&1' % make_basedir(nnn)
    verb = 'rebuilding'
    if not verb in the_time[builder]:
        the_time[builder][verb] = {}
    for i in range(repeats):
        os.system(rebuild)
        start = time.time()
        #print(cmd)
        assert(not os.system(cmd))
        stop = time.time()
        print '%s %s took %g seconds.' % (verb, builder, stop - start)
        the_time[builder][verb][nnn] = stop - start

    touch = 'echo >> %s/%s-generated.h' % (make_basedir(nnn), hashid(nnn))

    verb = 'touching-header'
    if not verb in the_time[builder]:
        the_time[builder][verb] = {}
    for i in range(repeats):
        os.system(touch)
        start = time.time()
        #print(cmd)
        assert(not os.system(cmd))
        stop = time.time()
        print '%s %s took %g seconds.' % (verb, builder, stop - start)
        the_time[builder][verb][nnn] = stop - start

    touch = 'echo >> %s/%s.c' % (make_basedir(nnn), hashid(1))

    verb = 'touching-c'
    if not verb in the_time[builder]:
        the_time[builder][verb] = {}
    for i in range(repeats):
        os.system(touch)
        start = time.time()
        #print(cmd)
        assert(not os.system(cmd))
        stop = time.time()
        print '%s %s took %g seconds.' % (verb, builder, stop - start)
        the_time[builder][verb][nnn] = stop - start

    verb = 'doing-nothing'
    if not verb in the_time[builder]:
        the_time[builder][verb] = {}
    for i in range(repeats):
        start = time.time()
        #print(cmd)
        assert(not os.system(cmd))
        stop = time.time()
        print '%s %s took %g seconds.' % (verb, builder, stop - start)
        the_time[builder][verb][nnn] = stop - start

tools = [cmd+' -j4' for cmd in ['make', 'bilge', 'tup', 'scons']]

all_nums_to_do = range(1, 11) + range(12, 25, 3) + range(30, 99, 10)
all_nums_to_do += [int(100*3**N) for N in range(1, 3)]
for nnn in all_nums_to_do:
    create_bench(nnn)

    print
    print 'timing number', nnn
    print
    for tool in tools:
        time_command(nnn, tool)
        print

legends = {
    'touching-header': 'touching header',
    'touching-c': 'touching c',
    'doing-nothing': 'doing nothing',
}

for verb in ['building', 'rebuilding', 'touching-header', 'touching-c', 'doing-nothing']:
    plt.figure()
    plt.title('Time spent '+verb)
    if verb in legends:
        plt.title('Time spent '+legends[verb])
    plt.xlabel('$N$')
    plt.ylabel('$t$ (s)')
    for cmd in tools:
        times = range(len(all_nums_to_do))
        for ii in range(len(all_nums_to_do)):
            times[ii] = 1.0*the_time[cmd][verb][all_nums_to_do[ii]]
        plt.loglog(all_nums_to_do, times, 'o-', label=cmd)
    plt.legend(loc='best')
    def cleanup(c):
        if c == ' ':
            return '-'
        return c
    plt.savefig('web/flat-dependent-'+verb+'.pdf')
    plt.savefig('web/flat-dependent-'+verb+'.png', dpi=100)

#plt.show()
