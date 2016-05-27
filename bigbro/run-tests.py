#!/usr/bin/python3

from __future__ import print_function

import glob, os, importlib, sys, time

if 'perf_counter' in dir(time):
    perf_counter = time.perf_counter
else:
    perf_counter = time.time

benchmark = 'bench' in sys.argv

platform = sys.platform
if platform == 'linux2':
    platform = 'linux'

assert not os.system('rm -rf tests/*.test')
numfailures = 0

def create_clean_tree(prepsh='this file does not exist'):
    os.system('rm -rf tmp*')
    os.mkdir('tmp')
    os.mkdir('tmp/subdir1')
    os.mkdir('tmp/subdir1/deepdir')
    os.mkdir('tmp/subdir2')
    os.system('ln -s ../subdir1 tmp/subdir2/symlink')
    os.system('ln -s `pwd` tmp/root_symlink')
    os.system('echo test > tmp/subdir2/test')
    os.system('echo foo > tmp/foo')
    os.system('ln -s ../foo tmp/subdir1/foo_symlink')
    if os.path.exists(prepsh):
        cmd = 'sh %s 2> %s.err 1> %s.out' % (prepsh, prepsh, prepsh)
        if os.system(cmd):
            os.system('cat %s.out' % prepsh);
            os.system('cat %s.err' % prepsh);
            print("prep command failed:", cmd)
            exit(1)

print('running C tests on bigbro:')
print('==========================')
for testc in glob.glob('tests/*.c'):
    base = testc[:-2]
    test = base+'.test'
    if '-static' in testc:
        if os.system('${CC-gcc} -Wall -static -O2 -o %s %s' % (test, testc)):
            print('%s fails to compile, skipping test' % testc)
            continue
    else:
        if os.system('${CC-gcc} -Wall -O2 -o %s %s' % (test, testc)):
            print('%s fails to compile, skipping test' % testc)
            continue
    create_clean_tree()
    before = perf_counter()
    cmd = './bigbro %s 2> %s.err 1> %s.out' % (test, base, base)
    if os.system(cmd):
        os.system('cat %s.out' % base);
        os.system('cat %s.err' % base);
        print("command failed:", cmd)
        exit(1)
    measured_time = perf_counter() - before
    err = open(base+'.err','r').read()
    out = open(base+'.out','r').read()
    m = importlib.import_module('tests.'+base[6:])
    # print(err)
    if benchmark:
        create_clean_tree()
        before = perf_counter()
        cmd = '%s 2> %s.err 1> %s.out' % (test, base, base)
        os.system(cmd)
        reference_time = perf_counter() - before
        if measured_time < 1e-3:
            time_took = '(%g vs %g us)' % (measured_time*1e6, reference_time*1e6)
        elif measured_time < 1:
            time_took = '(%g vs %g ms)' % (measured_time*1e3, reference_time*1e3)
        else:
            time_took = '(%g vs %g s)' % (measured_time, reference_time)
    else:
        if measured_time < 1e-3:
            time_took = '(%g us)' % (measured_time*1e6)
        else:
            time_took = '(%g ms)' % (measured_time*1e3)
    if m.passes(out, err):
        print(test, "passes", time_took)
    else:
        print(test, "FAILS!", time_took)
        numfailures += 1

test = None # to avoid bugs below where we refer to test
print()
print('running sh tests on bigbro:')
print('===========================')
for testsh in glob.glob('tests/*.sh'):
    base = testsh[:-3]
    create_clean_tree(testsh+'.prep')
    before = perf_counter()
    cmd = './bigbro sh %s 2> %s.err 1> %s.out' % (testsh, base, base)
    if os.system(cmd):
        os.system('cat %s.out' % base);
        os.system('cat %s.err' % base);
        print("command failed:", cmd)
        exit(1)
    measured_time = perf_counter() - before
    err = open(base+'.err','r').read()
    out = open(base+'.out','r').read()
    if benchmark:
        create_clean_tree(testsh+'.prep')
        before = perf_counter()
        cmd = 'sh %s 2> %s.err 1> %s.out' % (testsh, base, base)
        os.system(cmd)
        reference_time = perf_counter() - before
        if measured_time < 1e-3:
            time_took = '(%g vs %g us)' % (measured_time*1e6, reference_time*1e6)
        elif measured_time < 1:
            time_took = '(%g vs %g ms)' % (measured_time*1e3, reference_time*1e3)
        else:
            time_took = '(%g vs %g s)' % (measured_time, reference_time)
    else:
        if measured_time < 1e-3:
            time_took = '(%g us)' % (measured_time*1e6)
        else:
            time_took = '(%g ms)' % (measured_time*1e3)
    m = importlib.import_module('tests.'+base[6:])
    # print(err)
    if m.passes(out, err):
        print(testsh, "passes", time_took)
    else:
        print(testsh, "FAILS!", time_took)
        numfailures += 1

if benchmark:
    print()
    print('running sh benchmarks:')
    print('======================')
    for testsh in glob.glob('bench/*.sh'):
        base = testsh[:-3]
        create_clean_tree(testsh+'.prep')
        before = perf_counter()
        cmd = './bigbro sh %s 2> %s.err 1> %s.out' % (testsh, base, base)
        if os.system(cmd):
            os.system('cat %s.out' % base);
            os.system('cat %s.err' % base);
            print("command failed:", cmd)
            exit(1)
        measured_time = perf_counter() - before
        # The first time is just to warm up the file system cache...

        create_clean_tree(testsh+'.prep')
        before = perf_counter()
        cmd = 'sh %s 2> %s.err 1> %s.out' % (testsh, base, base)
        os.system(cmd)
        reference_time = perf_counter() - before
        before = perf_counter()
        cmd = './bigbro sh %s 2> %s.err 1> %s.out' % (testsh, base, base)
        os.system(cmd)
        measured_time = perf_counter() - before
        if measured_time < 1e-3:
            time_took = '(%g vs %g us)' % (measured_time*1e6, reference_time*1e6)
        elif measured_time < 1:
            time_took = '(%g vs %g ms)' % (measured_time*1e3, reference_time*1e3)
        else:
            time_took = '(%g vs %g s)' % (measured_time, reference_time)
        print(testsh, time_took)

if numfailures > 0:
    print("\nTests FAILED!!!")
exit(numfailures)
