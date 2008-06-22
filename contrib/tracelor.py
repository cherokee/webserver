#!/usr/bin/env python

##
## Cherokee trace colorizer
##
## If cherokee is compiled with the --enable-trace parameter, it can
## be traced by using the CHEROKEE_TRACE environment variable or
## cherokee-tweak.
##
## Copyright: Alvaro Lopez Ortega <alvaro@alobbs.com>
## Licensed: GPL v2
##

import sys

RESET_COLOR = "\033[0m"
HIGHLIGHT   = "\033[1;31;40m"

_threads = {}
_colors  = []
_color_n =  0

def build_colors():
    global _colors
    for a in range(40,48):
        for b in range(30,38):
            if a-10 == b:
                continue
            color = '\033[0;%d;%dm' % (a,b)
            _colors.append(color)
    
def thread_color (thread):
    global _threads
    global _color_n
    if not thread in _threads.keys():
        color = _colors[_color_n]
        _threads[thread] = color
        _color_n += 1
    return _threads[thread]

def main():
    build_colors()

    while True:
        line = sys.stdin.readline()
        if len(line) < 1:
            break

        # Thread
        thread = None
        if line[0] == '{':        
            end = line.find('}')
            if end > 0:
                thread = line[1:end]
                color  = thread_color (thread)
                line = '%s%s%s %s' % (color, thread, RESET_COLOR, line[end+2:])

        # Words
        for w in sys.argv:
            line = line.replace(w, HIGHLIGHT + w + RESET_COLOR)

        # Nothing else to do..
        print line,

if __name__ == '__main__':
    main()
