#!/usr/bin/python3
from __future__ import print_function

import os, sys

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

import cats
import hierarchy
import dependentchains
import sleepy
import independent

#matplotlib.rc('font', size='16.0')

datadir = os.getcwd()+'/data/'
modules = [dependentchains, cats, hierarchy, sleepy, independent]

allcolors = ['r','b','g','k','c','y','m', 'r']
allpatterns = ['o-', 's:', '*-.', 'x--', '.-', '<-', '>-', 'v-']

tool_patterns = {}
fslabels, fshandles = [], []
toollabels, toolhandles = [], []

mod = None
for m in modules:
    if m.name == sys.argv[1]:
        mod = m
if mod is None:
    print("Invalid mod: ", sys.argv[1])
    exit(1)

verb = sys.argv[2]
if verb not in mod.verbs:
    print("Invalid verb: ", sys.argv[2])
    exit(1)

dates = os.listdir(datadir)
dates.sort()
date = dates[-1]
# os.chdir(datadir+date+'/'+mod.name)
print('date', date, mod.name)

plt.figure(figsize=(6,4.3))
plt.title('%s %s on %s' % (verb, mod.name, date))
have_handled = {}

num_fs = len(os.listdir(datadir+date+'/'+mod.name+'/fac -j4'))

tools = os.listdir(datadir+date+'/'+mod.name)
tools.sort()
for tool in tools:
    if not tool in tool_patterns:
        tool_patterns[tool] = allpatterns[0]
        allpatterns = allpatterns[1:]
        if num_fs == 1:
            mycolor = allcolors[0]
            allcolors = allcolors[1:]
        toollabels.append(tool)
        toolhandles.append(plt.Line2D((0,1),(0,0), marker=tool_patterns[tool][0],
                                  linestyle=tool_patterns[tool][1:], color='k'))
    for fs in os.listdir(datadir+date+'/'+mod.name+'/'+tool):
        if num_fs > 1 and not fs in fs_colors:
            mycolor = allcolors[0]
            allcolors = allcolors[1:]
            fslabels.append(fs)
            fshandles.append(plt.Line2D((0,1),(0,0), color=fs_colors[fs], linewidth=3))
        data = np.loadtxt(datadir+date+'/'+mod.name+'/'+tool+'/'+fs+'/'+verb+'.txt')
        # The folowing few lines handles the case where we
        # have run the benchmark a few times, and have
        # redundant data.  We sort it, and then replace all
        # the points with a given N with the average of all
        # the measurements (from that date).
        if len(data.shape) == 2:
            for n in data[:,0]:
                ind = data[:,0] == n
                data[ind,1] = np.mean(data[ind,1])
            data = np.sort(np.vstack({tuple(row) for row in data}), axis=0) # remove duplicate lines
            if num_fs > 1:
                mylabel = '%s on %s' % (tool, fs)
            else:
                mylabel = tool
            plt.loglog(data[:,0], data[:,1]/data[:,0],
                       tool_patterns[tool],
                       color=mycolor,
                       label=mylabel)
plt.gca().grid(True)
plt.xlabel('$N$')
plt.ylabel('$t/N$ (s)')
if num_fs > 1:
    plt.legend(fshandles+toolhandles, fslabels+toollabels, loc='best', frameon=False)
else:
    plt.legend(loc='best', frameon=False)

plt.tight_layout()
# plt.savefig('../web/%s-%s.pdf' % (mod.name, verb))
plt.savefig('../web/%s-%s.svg' % (mod.name, verb), dpi=60)
# plt.savefig('../web/%s-%s.png' % (mod.name, verb), dpi=100)
