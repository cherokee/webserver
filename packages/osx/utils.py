import os
import subprocess


# Colors
#
ESC   = chr(27) + '['
RESET = '%s0m' % (ESC)

def green (s):
    return ESC + '0;32m' + s + RESET

def red (s):
    return ESC + '0;31m' + s + RESET

def yellow (s):
    return ESC + '1;33m' + s + RESET
    
def blue (s):
    return ESC + '0;34m' + s + RESET


# Utilies
#
def prepend_env (var, value):
    prev = os.getenv(var)
    os.putenv(var, "%s:%s"%(value, prev))
    return prev

def chdir (dir):
    if '*' in dir or '?' in dir:
        tmp = exe_output ("ls -1d %s" %(dir))
        dir = tmp.split('\n')[0]
    current = os.getcwd()
    os.chdir (dir)
    return current

def exe_output (cmd):
    p = subprocess.Popen (cmd, shell=True, stdout=subprocess.PIPE)
    return p.stdout.read()

def exe (cmd, colorer=lambda x: x, return_fatal=True):
    p = subprocess.Popen (cmd, shell=True, stdout=subprocess.PIPE)
    while True:
        line = p.stdout.readline()
        if not line:
            break
        print colorer(line.rstrip('\n\r'))

    p.wait()
    if return_fatal:
        if p.returncode != 0:
            print '\n%s: Could execute: %s' %(red('ERROR'), cmd)

        assert p.returncode == 0, "Execution failed"

    return p.returncode

def which (program):
    def is_exe(fpath):
        return os.path.exists(fpath) and os.access(fpath, os.X_OK)

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None
