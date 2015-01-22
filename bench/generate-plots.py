#!/usr/bin/python2

import os

import matplotlib.pyplot as plt
import numpy as np

import catmod

datadir = os.getcwd()+'/bench/data/'
modules = [catmod]

allcolors = ['r','b','g','k','c','y']
allpatterns = ['o-', 'x--', '*:', '.-', '<-', '>-', 'v-']

fs_colors = {}
tool_patterns = {}
fslabels, fshandles = [], []
toollabels, toolhandles = [], []

for mod in modules:
    dates = os.listdir(datadir)
    dates.sort()
    date = dates[-1]
    # os.chdir(datadir+date+'/'+mod.name)
    print 'date', date

    for verb in mod.verbs:
        plt.figure()
        plt.title('%s %s on %s' % (verb, mod.name, date))
        have_handled = {}

        for tool in os.listdir(datadir+date+'/'+mod.name):
            if not tool in tool_patterns:
                tool_patterns[tool] = allpatterns[0]
                allpatterns = allpatterns[1:]
                toollabels.append(tool)
                toolhandles.append(plt.Line2D((0,1),(0,0), marker=tool_patterns[tool][0],
                                          linestyle=tool_patterns[tool][1:], color='k'))
            for fs in os.listdir(datadir+date+'/'+mod.name+'/'+tool):
                if not fs in fs_colors:
                    fs_colors[fs] = allcolors[0]
                    allcolors = allcolors[1:]
                    fslabels.append(fs)
                    fshandles.append(plt.Line2D((0,1),(0,0), color=fs_colors[fs], linewidth=3))
                data = np.loadtxt(datadir+date+'/'+mod.name+'/'+tool+'/'+fs+'/'+verb+'.txt')
                plt.loglog(data[:,0], data[:,1],
                           tool_patterns[tool],
                           color=fs_colors[fs],
                           label='%s on %s' % (tool, fs))
        plt.legend(fshandles+toolhandles, fslabels+toollabels, loc='best')
        plt.savefig('web/%s-%s.pdf' % (mod.name, verb))
