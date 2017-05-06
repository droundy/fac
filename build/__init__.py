import time

previous_time = 0

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'

def blue(x):
    return bcolors.OKBLUE + x + bcolors.ENDC
def red(x):
    return bcolors.FAIL + x + bcolors.ENDC
def green(x):
    return bcolors.OKGREEN + x + bcolors.ENDC
def warn(x):
    return bcolors.WARNING + x + bcolors.ENDC

PASS = green('PASS')
FAIL = red('FAIL')

def elapsed_time():
    global previous_time;
    if previous_time == 0:
        previous_time = time.monotonic()
        return 0
    now = time.monotonic()
    elapsed = now - previous_time
    previous_time = now
    return elapsed

def took(job=''):
    colorme = blue
    t = elapsed_time()
    if t >= 60:
        colorme = red
    elif t < 1:
        colorme = green
    elif t > 10:
        colorme = warn
    if len(job)>1 and job[-1] != ' ':
        return job + colorme(' took %g s' % t)
    return job + colorme('%g s' % t)
