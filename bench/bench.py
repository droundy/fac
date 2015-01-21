#!/usr/bin/python2

import os, hashlib, time, numpy, sys, datetime
import matplotlib.pyplot as plt

extra_name = ''
if len(sys.argv) == 2:
    extra_name = sys.argv[1]
    print 'extra_name is', extra_name

date = str(datetime.datetime.now())[:10]
print 'date is', date

def make_basedir(n):
    return 'bench/temp/test-%d/' % n

def make_name(i, n):
    path = '.'
    digits = '%d' % i
    for d in digits:
        path = path+'/number-'+d
    return path[2:]+('_%d' % i)

def make_path(i, n):
    path = 'bench/temp'
    digits = '%d' % i
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

os.system('rm -rf bench/temp')

def create_bench(n):
    N = 0 # number of actual C files
    make_path(0, n)
    sconsf = open(make_basedir(n)+'SConstruct', 'w')
    bilgef = open(make_basedir(n)+'top.bilge', 'w')
    tupf = open(make_basedir(n)+'Tupfile', 'w')
    makef = open(make_basedir(n)+'Makefile', 'w')
    open(make_basedir(n)+'Tupfile.ini', 'w')
    sconsf.write("""
env = Environment(CPPPATH=['.'])
""")
    makef.write('all:')
    for i in range(10**n):
        makef.write(' %s.o' % make_name(i, n))
    makef.write("""

clean:
\trm -f *.o */*.o */*/*.o */*/*/*.o */*/*/*/*.o
""")
    for i in range(10**n):
        make_path(i, n)
        base = 'bench/temp/test-%d/' % n + make_name(i, n)
        cname = make_name(i, n) + '.c'
        hname = make_name(i, n) + '.h'
        includes = ''
        funcs = ''
        include_deps = ''
        for xx in [i+ii for ii in range(10)]:
            includes += '#include "%s.h"\n' % make_name(hash_integer(xx) % 10**n, n)
            funcs += '    %s();\n' % hashid(hash_integer(xx) % 10**n)
            include_deps += ' %s.h' % make_name(hash_integer(xx) % 10**n, n)
        makef.write("""
# %d
%s.o: %s.c %s
\tgcc -Wall -Werror -I. -O2 -c -o %s.o %s.c
""" % (i, make_name(i, n), make_name(i, n), include_deps,
       make_name(i, n), make_name(i, n)))
        tupf.write("""
# %d
: %s.c |> gcc -I. -O2 -c -o %%o %%f |> %s.o
""" % (i, make_name(i, n), make_name(i, n)))
        bilgef.write("""
# %d
| gcc -I. -O2 -c -o %s.o %s.c
> %s.o
""" % (i, make_name(i, n), make_name(i, n), make_name(i, n)))
        sconsf.write("""
env.Object('%s.c')
""" % make_name(i,n))
        N += 1 # I'm about to make another file!!!
        f = open(base+'.c', 'w')
        f.write('\n')
        f.write("""/* c file %s */

%s#include "%s"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <limits.h>
#include <complex.h>
#include <time.h>

void delete_from_%s(list%s **list, const char *path) {
  while (*list != NULL) {
    if (strcmp((*list)->path, path) == 0) {
      list%s *to_be_deleted = *list;
      *list = (*list)->next;
      free(to_be_deleted->path);
      free(to_be_deleted);
      return;
    }
    list = &((*list)->next);
  }
}

int is_in_%s(const list%s *ptr, const char *path) {
  while (ptr != NULL) {
    if (strcmp(ptr->path, path) == 0) {
      return 1;
    }
    ptr = ptr->next;
  }
  return 0;
}

void insert_to_%s(list%s **list, const char *path) {
  delete_from_%s(list, path);
  list%s *newhead = (list%s *)malloc(sizeof(list%s));
  newhead->next = *list;
  newhead->path = malloc(strlen(path)+1);
  strcpy(newhead->path, path);

  *list = newhead;
}

void free_%s(list%s *list) {
  while (list != NULL) {
    list%s *d = list;
    list = list->next;
    free(d->path);
    free(d);
  }
}

static void stupid_this(char *s) {
  while (*s) {
    while (*s < 'a') {
      *s += 13;
    }
    while (*s > 'z') {
      *s -= 13;
    }
    printf("%%c", *s);
    s++;
  }
  printf("\\n");
}

void %s() {
    stupid_this("Hello world %s!\\n");
%s}
""" % (cname, includes, hname, hashid(i), hashid(i),
       hashid(i), hashid(i), hashid(i), hashid(i), hashid(i),
       hashid(i), hashid(i), hashid(i), hashid(i), hashid(i),
       hashid(i), hashid(i), hashid(i), hashid(i),
       funcs))
        f.close()
        f = open(base+'.h', 'w')
        f.write("""/* header %s */
#ifndef %s_H
#define %s_H

typedef struct list%s {
  char *path;
  struct list%s *next;
} list%s;

void insert_to_%s(list%s **list, const char *path);
void delete_from_%s(list%s **list, const char *path);
int is_in_%s(const list%s *list, const char *path);

void free_%s(list%s *list);

void %s();

#endif
""" % (hname,
       hashid(i), hashid(i), hashid(i), hashid(i), hashid(i),
       hashid(i), hashid(i), hashid(i), hashid(i), hashid(i),
       hashid(i), hashid(i), hashid(i), hashid(i)))
        f.close()
    return N

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

    rebuild = 'cd %s && sleep 2 && touch *.c */*.c */*/*.c */*/*/*.c */*/*/*/*.c > touch_output 2>&1' % make_basedir(nnn)
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

    touch = 'echo >> %s/number-0.h' % make_basedir(nnn)

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

    touch = 'echo >> %s/number-0.c' % make_basedir(nnn)

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
nmax = 4
Ns = range(0, nmax+1)
for nnn in range(0, nmax+1):
    Ns[nnn] = create_bench(nnn)

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
    plt.title('Time spent '+verb+' hierarchy (%s)' % date)
    if verb in legends:
        plt.title('Time spent '+legends[verb]+' hierarchy (%s)' % date)
    plt.xlabel('$\log_{10}N$')
    plt.ylabel('$t$ (s)')
    for cmd in tools:
        nums = range(0,nmax+1)
        times = range(0,nmax+1)
        for n in nums:
            times[n] = 1.0*the_time[cmd][verb][n]
        plt.loglog(Ns, times, 'o-', label=cmd)
    plt.legend(loc='best')
    def cleanup(c):
        if c == ' ':
            return '-'
        return c
    plt.savefig('web/hierarchical-'+verb+extra_name+'.pdf')
    plt.savefig('web/hierarchical-'+verb+extra_name+'.png', dpi=100)

#plt.show()
