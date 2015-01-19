#!/usr/bin/python2

import os, hashlib, time, numpy
import matplotlib.pyplot as plt

def make_basedir(n):
    return 'bench/temp-cat/cat-%d/' % n

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

os.system('rm -rf bench/temp-cat')

def create_bench(n):
    try:
        os.mkdir('bench/temp-cat')
    except:
        pass
    os.mkdir(make_basedir(n))
    sconsf = open(make_basedir(n)+'SConstruct', 'w')
    bilgef = open(make_basedir(n)+'top.bilge', 'w')
    tupf = open(make_basedir(n)+'Tupfile', 'w')
    makef = open(make_basedir(n)+'Makefile', 'w')
    open(make_basedir(n)+'Tupfile.ini', 'w')
    sconsf.write("""
env = Environment()
""")
    makef.write("""
final.txt: %s.txt
\tcat $< > $@

""" % hashid(n))
    bilgef.write("""
| cat %s.txt > final.txt
> final.txt
< %s.txt

""" % (hashid(n), hashid(n)))
    f = open('bench/temp-cat/cat-%d/%s.txt' % (n, hashid(0)), 'w')
    f.write("Hello world\n")
    for i in range(1,n+1):
        base = 'bench/temp-cat/cat-%d/' % n + hashid(i)
        makef.write("""
# %d
%s.txt: %s.txt
\tcat $< > $@
""" % (i, hashid(i), hashid(i-1)))
        tupf.write("""
# %d
: %s.txt |> cat %%f > %%o |> %s.txt
""" % (i, hashid(i-1), hashid(i)))
        bilgef.write("""
# %d
| cat %s.txt > %s.txt
> %s.txt
< %s.txt
""" % (i, hashid(i-1), hashid(i), hashid(i), hashid(i-1)))
        sconsf.write("""
env.Command('%s.txt', '%s.txt', 'cat $SOURCE > $TARGET')
""" % (hashid(i), hashid(i-1)))

the_time = {}

def time_command(nnn, builder):
    global the_time
    if not builder in the_time:
        the_time[builder] = {}

    cmd = 'cd %s && %s' % (make_basedir(nnn), builder)
    cmd = 'cd %s && %s > output 2>&1' % (make_basedir(nnn), builder)

    cleanit = 'cd %s && mv %s.txt temp && rm -f *.txt && mv temp %s.txt' % (make_basedir(nnn),
                                                                            hashid(0), hashid(0))

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

    rebuild = 'cd %s && touch %s.txt' % (make_basedir(nnn), hashid(0))
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

all_nums_to_do = []
num_to_do = 1.7782795
while num_to_do < 101:
    all_nums_to_do.append(int(num_to_do))
    num_to_do *= 1.7782795

for nnn in all_nums_to_do:
    create_bench(nnn)

    print
    print 'timing number', nnn
    print
    for tool in tools:
        time_command(nnn, tool)
        print

legends = {
    'doing-nothing': 'doing nothing',
}

for verb in ['building', 'rebuilding', 'doing-nothing']:
    plt.figure()
    plt.title('Time spent '+verb+' cats')
    if verb in legends:
        plt.title('Time spent '+legends[verb]+' cats')
    plt.xlabel('$N$')
    plt.ylabel('$t$ (s)')
    for cmd in tools:
        times = range(len(all_nums_to_do))
        for ii in range(len(all_nums_to_do)):
            times[ii] = 1.0*the_time[cmd][verb][all_nums_to_do[ii]]
        plt.loglog(all_nums_to_do, times, 'o-', label=cmd)
    plt.legend(loc='best')
    plt.tight_layout()
    def cleanup(c):
        if c == ' ':
            return '-'
        return c
    plt.savefig('web/flat-cats-'+verb+'.pdf')
    plt.savefig('web/flat-cats-'+verb+'.png', dpi=100)

#plt.show()
