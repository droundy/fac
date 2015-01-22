#!/usr/bin/python3

import os, hashlib, time, numpy, sys, datetime

import catmod

date = str(datetime.datetime.now())[:10]

datadir = os.getcwd()+'/bench/data/'
os.makedirs(datadir, exist_ok=True)
modules = [catmod]

rootdirnames = ['home', 'tmp', 'vartmp']
rootdirs = {'home': os.getcwd()+'/bench/temp',
            'tmp': '/tmp/benchmarking',
            'vartmp': '/var/tmp/benchmarking'}

minute = 60
hour = 60*minute
time_limit = 60*minute

tools = [cmd+' -j4' for cmd in ['make', 'bilge', 'tup', 'scons']]

def find_mount_point(path):
    path = os.path.abspath(path)
    while not os.path.ismount(path):
        path = os.path.dirname(path)
    return path
def identify_filesystem(path):
    path = find_mount_point(path)
    with open('/proc/mounts') as f:
        x = f.read()
    x = x[10:] # to skip "rootfs / rootfs" which I do not understand.
    x = x[x.find(' '+path+' ')+len(path)+2:]
    return x[:x.find(' ')]

filesystems = {}
for r in rootdirnames:
    filesystems[r] = identify_filesystem(rootdirs[r])
print(filesystems)

def time_command(mod, builder):
    the_time = {}

    cmd = '%s > output 2>&1' % builder
    for verb in mod.verbs:
        if verb in mod.prepare():
            assert(not os.system(mod.prepare()[verb]))
        start = time.time()
        assert(not os.system(cmd))
        stop = time.time()
        print('%s %s took %g seconds.' % (verb, builder, stop - start))
        the_time[verb] = stop - start
    return the_time

for mod in modules:
    Ns = []
    Nfloat = 1.7782795
    start_module = time.time()
    while time.time() < start_module + time_limit:
        N = int(Nfloat)
        Ns.append(N)
        Nfloat *= 1.7782795
        print('\nWorking with N = %d' % N)

        for rootdirname in rootdirnames:
            rootdir = rootdirs[rootdirname]
            print('\n### Using rootdir', rootdirname)
            os.makedirs(rootdir, exist_ok=True)
            os.chdir(rootdir)
            for tool in tools:
                print('')
                os.system('rm -rf '+mod.name+'-%d'%N)
                os.makedirs(mod.name+'-%d'%N)
                os.chdir(mod.name+'-%d'%N)
                mod.create_bench(N)
                times = time_command(mod, tool)
                os.chdir('..')
                datadirname = datadir+'/%s/%s/%s/%s/' % (date, mod.name, tool, filesystems[rootdirname])
                os.makedirs(datadirname, exist_ok=True)
                for verb in mod.verbs:
                    with open(datadirname+verb+'.txt', 'a') as f:
                        f.write('%d\t%g\n' % (N, times[verb]))
