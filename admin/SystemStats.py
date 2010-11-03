# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2010 Alvaro Lopez Ortega
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#

import os
import re
import sys
import time
import subprocess

from threading import Thread

#
# Factory function
#
_stats = None
def get_system_stats():
    global _stats

    if not _stats:
        if sys.platform == 'linux2':
            _stats = System_stats__Linux()
        elif sys.platform == 'darwin':
            _stats = System_stats__Darwin()

    assert _stats, "Not implemented"
    return _stats


# Base class
class System_stats:
    class CPU:
        def __init__ (self):
            self.user  = 0
            self.sys   = 0
            self.idle  = 0
            self.usage = 0

            self.speed = ''
            self.num   = ''
            self.cores = ''

    class Memory:
        def __init__ (self):
            self.total = 0
            self.used  = 0
            self.free  = 0

    def __init__ (self):
        self.cpu      = self.CPU()
        self.mem      = self.Memory()
        self.hostname = ''


# MacOS X implementation
class System_stats__Darwin (Thread, System_stats):
    CHECK_INTERVAL = 2

    def __init__ (self):
        Thread.__init__ (self)
        System_stats.__init__ (self)

        # System Profiler
        self.profiler = subprocess.Popen ("/usr/sbin/system_profiler SPHardwareDataType",
                                           shell=True, stdout = subprocess.PIPE)

        # vm_stat (and skip the two first lines)
        self.vm_stat_fd = subprocess.Popen ("/usr/bin/vm_stat %d" %(self.CHECK_INTERVAL),
                                            shell=True, stdout = subprocess.PIPE)

        line = self.vm_stat_fd.stdout.readline()
        self._page_size = int (re.findall("page size of (\d+) bytes", line)[0])

        first_line = self.vm_stat_fd.stdout.readline()
        if 'spec' in first_line:
            # free active spec inactive wire faults copy 0fill reactive pageins pageout
            self.vm_stat_type = 11
        else:
            # free active inac wire faults copy zerofill reactive pageins pageout
            self.vm_stat_type = 10

        # I/O stat
        self.iostat_fd = subprocess.Popen ("/usr/sbin/iostat -n 0 -w %d" %(self.CHECK_INTERVAL),
                                           shell=True, stdout = subprocess.PIPE)

        # Read valid values
        self._read_hostname()
        self._read_cpu()
        self._read_memory()
        self._read_profiler()

        self.start()

    def _read_hostname (self):
        fd = subprocess.Popen ("/bin/hostname", shell=True, stdout = subprocess.PIPE)
        self.hostname = fd.stdout.readline().strip()

    def _read_profiler (self):
        tmp = self.profiler.stdout.read()
        self.cpu.speed = re.findall (r'Processor Speed: (.*?)\n',     tmp)[0]
        self.cpu.num   = re.findall (r'Number Of Processors: (\d+)',  tmp)[0]
        self.cpu.cores = re.findall (r'Total Number Of Cores: (\d+)', tmp)[0]

    def _read_cpu (self):
        # Read a new line
        line = self.iostat_fd.stdout.readline().rstrip('\n')

        # Skip headers
        if len(filter (lambda x: x not in " .0123456789", line)):
            return

        # Parse
        parts = filter (lambda x: x, line.split(' '))
        assert len(parts) == 6, parts

        self.cpu.user  = int(parts[0])
        self.cpu.sys   = int(parts[1])
        self.cpu.idle  = int(parts[2])
        self.cpu.usage = 100 - self.cpu.idle

    def _read_memory (self):
        def to_int (x):
            if x[-1] == 'K':
                return long(x[:-1]) * 1024
            return long(x)

        line = self.vm_stat_fd.stdout.readline().rstrip('\n')

        # Skip headers
        if len(filter (lambda x: x not in " .0123456789", line)):
            return

        # Parse
        tmp = filter (lambda x: x, line.split(' '))
        values = [(to_int(x) * self._page_size) / 1024 for x in tmp]

        if self.vm_stat_type == 11:
            # free active spec inactive wire faults copy 0fill reactive pageins pageout
            free, active, spec, inactive, wired, faults, copy, fill, reactive, pageins, pageout = values
            self.mem.total = free + active + spec + inactive + wired
        elif self.vm_stat_type == 10:
            # free active inac wire faults copy zerofill reactive pageins pageout
            free, active, inactive, wired, faults, copy, fill, reactive, pageins, pageout = values
            self.mem.total = free + active + inactive + wired

        self.mem.free  = (free + inactive)
        self.mem.used  = self.mem.total - self.mem.free

    def run (self):
        while True:
            self._read_cpu()
            self._read_memory()


# Linux implementation
class System_stats__Linux (Thread, System_stats):
    CHECK_INTERVAL = 2

    def __init__ (self):
        Thread.__init__ (self)
        System_stats.__init__ (self)

        self.cpu._user_prev = 0
        self.cpu._sys_prev  = 0
        self.cpu._nice_prev = 0
        self.cpu._idle_prev = 0

        # Read valid values
        self._read_hostname()
        self._read_cpu()
        self._read_memory()
        self._read_cpu_info()

        self.start()

    def _read_hostname (self):
        # Read /etc/hostname
        if os.access ("/etc/hostname", os.R_OK):
            fd = open ("/etc/hostname", 'r')
            self.hostname = fd.readline().strip()
            fd.close()
            return

        # Execute /bin/hostname
        fd = subprocess.Popen ("/bin/hostname", shell=True, stdout = subprocess.PIPE)
        self.hostname = fd.stdout.readline().strip()

    def _read_cpu_info (self):
        fd = open("/proc/cpuinfo", 'r')
        tmp = fd.read()
        fd.close()

        # Cores
        cores = re.findall(r'cpu cores.+?(\d+)\n', tmp)
        if cores:
            self.cpu.cores = cores[0]

        # Processors
        self.cpu.num = str (len(re.findall (r'processor.+?(\d+)\n', tmp)))

        # Speed
        hz = re.findall (r'model name.+?([\d. ]+GHz)\n', tmp)
        if not hz:
            hz = re.findall (r'model name.+?([\d. ]+MHz)\n', tmp)
            if not hz:
                hz = re.findall (r'model name.+?([\d. ]+THz)\n', tmp)

        if hz:
            self.cpu.speed = hz[0]
        else:
            mhzs = re.findall (r'cpu MHz.+?([\d.]+)\n', tmp)
            self.cpu.speed = '%s MHz' %(' + '.join(mhzs))

    def _read_cpu (self):
        fd = open("/proc/stat", 'r')
        fields = fd.readline().split()
        fd.close()

        user = float(fields[1])
        sys  = float(fields[2])
        nice = float(fields[3])
        idle = float(fields[4])

        total = ((user - self.cpu._user_prev) + (sys - self.cpu._sys_prev) + (nice - self.cpu._nice_prev) + (idle - self.cpu._idle_prev))
        self.cpu.usage = int(100.0 * ((user + sys + nice) - (self.cpu._user_prev + self.cpu._sys_prev + self.cpu._nice_prev)) / (total + 0.001) + 0.5)

        if (self.cpu.usage > 100):
            self.cpu.usage = 100

        self.cpu.idle = 100 - self.cpu.usage

        self.cpu._user_prev = user
        self.cpu._sys_prev  = sys
        self.cpu._nice_prev = nice
        self.cpu._idle_prev = idle

    def _read_memory (self):
        fd = open("/proc/meminfo", "r")
        lines = fd.readlines()
        fd.close()

        total   = 0
        used    = 0
        cached  = 0
        buffers = 0

        for line in lines:
            parts = line.split()
            if parts[0] == 'MemTotal:':
                total = int(parts[1])
            elif parts[0] == 'MemFree:':
                used = int(parts[1])
            elif parts[0] == 'Cached:':
                cached = int(parts[1])
            elif parts[0] == 'Buffers:':
                buffers = int(parts[1])

        self.mem.total = total
        self.mem.used  = total - (used + cached + buffers)
        self.mem.free  = total - self.mem.used

    def run (self):
        while True:
            self._read_cpu()
            self._read_memory()
            time.sleep (self.CHECK_INTERVAL)


if __name__ == '__main__':
    sys_stats = get_system_stats()

    print "Hostname:",   sys_stats.hostname
    print "Speed:",      sys_stats.cpu.speed
    print "Processors:", sys_stats.cpu.num
    print "Cores:",      sys_stats.cpu.cores

    while True:
        print "CPU:",
        print 'used', sys_stats.cpu.usage,
        print 'idle', sys_stats.cpu.idle

        print "MEMORY:",
        print 'total', sys_stats.mem.total,
        print 'used',  sys_stats.mem.used,
        print 'free',  sys_stats.mem.free

        time.sleep(1)
