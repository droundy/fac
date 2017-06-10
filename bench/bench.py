#!/usr/bin/python3

import os, hashlib, time, numpy, sys, datetime, subprocess

import cats
import hierarchy
import dependentchains
import sleepy
import independent

minute = 60
hour = 60*minute
day = 24*hour
time_limit = 6*day

tools = [cmd+' -j4' for cmd in ['make', 'fac', 'fac --blind', 'tup', 'scons']] # + ['sh build.sh']

if os.system('rust-fac --version') == 0:
    tools.append('rust-fac -j4')
    tools.append('rust-fac --blind -j4')
    tools.append('ninja -j4')
    tools = sorted(tools)

# The variable "date" actually contains the date and short hash of the
# commit information.  This could lead to confusion and incorrectness
# if we run benchmarking without actually committing, but if used
# properly it should enable us to more accurately track which version
# of the code corresponded to which benchmarking data.

# Also note that this will fail if we checkout an old version of the
# code.

date = subprocess.check_output(['git', 'log', '--pretty=%ci', '-n',  '1'], stderr=subprocess.STDOUT).decode('utf-8')
date = date[:10]+'_'
date += subprocess.check_output(['git', 'log', '--pretty=%h', '-n',  '1'], stderr=subprocess.STDOUT).decode('utf-8')[:-1]

datadir = os.getcwd()+'/bench/data/'
os.makedirs(datadir, exist_ok=True)
modules = [sleepy, dependentchains, hierarchy, cats, independent]

rootdirnames = ['tmp', 'home' , 'vartmp']
rootdirs = {'home': os.getcwd()+'/bench/temp',
            'tmp': '/tmp/benchmarking',
            'vartmp': '/var/tmp/benchmarking'}

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

tmprootdirnames = rootdirnames
rootdirnames = []
filesystems = {}
for r in tmprootdirnames:
    fs = identify_filesystem(rootdirs[r])
    if fs in filesystems.values():
        print(fs,'is already covered')
    else:
        filesystems[r] = fs
        rootdirnames.append(r)
print(filesystems)

def time_command(mod, builder):
    the_time = {}

    cmd = '%s' % builder
    cmd = '%s > output 2>&1' % builder
    for verb in mod.verbs:
        if verb in mod.prepare():
            assert(not os.system(mod.prepare()[verb]))
        start = time.time()
        if not os.system(cmd):
            stop = time.time()
            print('%s %s took %g seconds.' % (verb, builder, stop - start))
            the_time[verb] = stop - start
        else:
            stop = time.time()
            print('%s %s failed in %g seconds.' % (verb, builder, stop - start))
    return the_time

Ns = []
Nfloat = 10.0
if len(sys.argv) > 1:
    Nfloat = float(sys.argv[1])
start_benchmarking = time.time()
while time.time() < start_benchmarking + time_limit:
    N = int(Nfloat)
    Ns.append(N)
    Nfloat *= 1.7782795
    for mod in modules:
        if mod.name == 'sleepy' and N not in [56, 100, 177]:
            continue
        print('\nWorking on %s with N = %d' % (mod.name, N))
        for rootdirname in rootdirnames:
            rootdir = rootdirs[rootdirname]
            print('\n### Using rootdir', rootdirname)
            os.makedirs(rootdir, exist_ok=True)
            os.chdir(rootdir)
            # here we create the source directory
            os.system('rm -rf '+mod.name+'-%d'%N)
            os.makedirs(mod.name+'-%d'%N)
            os.chdir(mod.name+'-%d'%N)
            assert(not os.system('git init'))
            mod.create_bench(N)
            assert(not os.system('fac --makefile Makefile --script build.sh --tupfile Tupfile > /dev/null && fac -c > /dev/null'))
            if 'rust-fac -j4' in tools:
                assert(not os.system('rust-fac --ninja build.ninja > /dev/null && rust-fac -c > /dev/null'))
            os.chdir('..')
            for tool in tools:
                print('')
                # we do each tool's build in a fresh copy of the source
                os.system('rm -rf working && cp -a %s-%d working' % (mod.name, N))
                os.chdir('working')
                times = time_command(mod, tool)
                os.chdir('..')
                datadirname = datadir+'/%s/%s/%s/%s/' % (date, mod.name, tool, filesystems[rootdirname])
                os.makedirs(datadirname, exist_ok=True)
                for verb in times.keys():
                    with open(datadirname+verb+'.txt', 'a') as f:
                        f.write('%d\t%g\n' % (N, times[verb]))
