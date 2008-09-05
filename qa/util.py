import os, sys, time, random, fcntl

from conf import *

term = os.getenv("TERM")
if 'color' in term:
    MESSAGE_SUCCESS = '\033[0;48;36m Success \033[0m' # Blue
    MESSAGE_FAILED  = '\033[0;48;31m  Failed \033[0m' # Red
    MESSAGE_SKIPPED = '\033[0;48;33m Skipped \033[0m' # Yellow
else:
    MESSAGE_SUCCESS = 'Success'
    MESSAGE_FAILED  = ' Failed'
    MESSAGE_SKIPPED = 'Skipped'


def count_down (msg, nsecs, nl=True):
    for s in range(nsecs):
        sys.stdout.write ((msg+'\r') % (nsecs - s - 1))
        sys.stdout.flush()
        time.sleep(1)
    if nl:
        print

def str_random_generate (n):
    c = ""
    x = int (random.random() * 100)
    y = int (random.random() * 100)
    z = int (random.random() * 100)

    for i in xrange(n):
        x = (171 * x) % 30269
        y = (172 * y) % 30307
        z = (170 * z) % 30323
        c += chr(32 + int((x/30269.0 + y/30307.0 + z/30323.0) * 1000 % 95))

    return c

def letters_random_generate (n):
    c = ""
    x = int (random.random() * 100)
    y = int (random.random() * 100)
    z = int (random.random() * 100)

    for i in xrange(n):
        x = (171 * x) % 30269
        y = (172 * y) % 30307
        z = (170 * z) % 30323
        if z%2:
            c += chr(65 + int((x/30269.0 + y/30307.0 + z/30323.0) * 1000 % 25))
        else:
            c += chr(97 + int((x/30269.0 + y/30307.0 + z/30323.0) * 1000 % 25))
            
    return c


str_buf     = ""
letters_buf = ""

def letters_random (n):
    global letters_buf

    letters_len = len(letters_buf)
    if letters_len == 0:
        letters_random_generate (1000)
        letters_len = len(letters_buf)

    offset = random.randint (0, letters_len)

    if letters_len - offset > n:
        return letters_buf[offset:offset+n]

    tmp = letters_random_generate (n - (letters_len - offset))
    letters_buf += tmp

    return letters_buf[offset:]


def str_random (n):
    global str_buf

    str_len = len(str_buf)
    if str_len == 0:
        str_random_generate (1000)
        str_len = len(str_buf)

    offset = random.randint (0, str_len)

    if str_len - offset > n:
        return str_buf[offset:offset+n]

    tmp = str_random_generate (n - (str_len - offset))
    str_buf += tmp
    return str_buf[offset:]


def check_php_interpreter (fullpath):
    f = os.popen ("%s -v" % (fullpath))
    all = reduce (lambda x,y: x+y, f.readlines())
    f.close()
    return "cgi-fcgi" in all

__php_ref = None
def look_for_php():    
    global __php_ref

    if __php_ref != None:
        return __php_ref
    
    if PHPCGI_PATH != "auto":
        if check_php_interpreter (PHPCGI_PATH):
            __php_ref = PHPCGI_PATH
            return __php_ref

    for p in PHP_DIRS:
        for n in PHP_NAMES:
            php = os.path.join(p,n)
            if os.path.exists(php):
                if check_php_interpreter(php):
                    __php_ref = php
                    return php

    error = "Couldn't find a suitable PHP interpreter (with fastcgi support)"
    __php_ref = error
    return __php_ref


__python_ref = None
def look_for_python():
    global __python_ref

    if __python_ref != None:
        return __python_ref    

    if PYTHON_PATH != "auto":
        __python_ref = PYTHON_PATH
        return __python_ref

    for p in PYTHON_DIRS:
        for n in PYTHON_NAMES:
            py = os.path.join(p,n)
            if os.path.exists(py):
                __python_ref = py
                return py

    print "ERROR: Python interpreter not found"
    __python_ref = ''
    return __python_ref


def print_key (key, val):
    print "%10s: %s" % (key, val)


__free_port = 5000
def get_free_port():
    global __free_port
    __free_port += 1
    return __free_port


__next_source = 0
def get_next_source():
    global __next_source
    __next_source += 1
    return __next_source


#
# Plug-in checking
#

_server_info = None

def cherokee_get_server_info ():
    global _server_info

    if _server_info == None:
        try:
            f = os.popen ("%s -i" % (CHEROKEE_SRV_PATH))
            _server_info = f.read()
            f.close()
        except:
            pass

    return _server_info


_built_in_list      = []
_built_in_list_done = False


def cherokee_build_info_has (filter, module):
    # Let's see whether it's built-in
    global _built_in_list
    global _built_in_list_done

    if not _built_in_list_done:
        _built_in_list_done = True

        try:
            f = os.popen ("%s -i" % (CHEROKEE_SRV_PATH))
            cont = f.read()
            f.close()
        except:
            pass

        try:
            filter_string = " %s: " % (filter)
            for l in cont.split("\n"):
                if l.startswith(filter_string):
                    line = l.replace (filter_string, "")
                    break
            _built_in_list = line.split(" ")
        except:
            pass

    return module in _built_in_list

def cherokee_has_plugin (module):
    # Check for the dynamic plug-in
    try:
        mods = filter(lambda x: module in x, os.listdir(CHEROKEE_PLUGINDIR))
        if len(mods) >= 1:
            return True
    except:
        pass

    return cherokee_build_info_has ("Built-in", module)

def print_sec (content):
    fcntl.flock (sys.stdout, fcntl.LOCK_EX)
    print content
    fcntl.flock (sys.stdout, fcntl.LOCK_UN)
