#!/usr/bin/python2

import os

import matplotlib.pyplot as plt
import numpy as np

import catmod

datadir = os.getcwd()+'/bench/data/'
modules = [catmod]

for mod in modules:
    dates = os.listdir(datadir)
    dates.sort()
    date = dates[-1]
    # os.chdir(datadir+date+'/'+mod.name)
    print 'date', date

    for verb in mod.verbs:
        plt.figure()
        plt.title('%s %s on %s' % (verb, mod.name, date))

        for tool in os.listdir(datadir+date+'/'+mod.name):
            for fs in os.listdir(datadir+date+'/'+mod.name+'/'+tool):
                data = np.loadtxt(datadir+date+'/'+mod.name+'/'+tool+'/'+fs+'/'+verb+'.txt')
                plt.loglog(data[:,0], data[:,1],
                           label='%s on %s' % (tool, fs))

        plt.legend(loc='best')
        plt.savefig('web/%s-%s.pdf' % (mod.name, verb))
