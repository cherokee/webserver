# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
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

import CTK
import SystemStats
from CTK.PageCleaner import Uniq_Block


JS_COMMON = """
var system_stats = {};

function update_system_stats() {
  $.ajax({
     type:     "GET",
     url:      "/system/stats",
     dataType: "json",
     async:     true,
     success: function (data){
        // deep copy
        system_stats = jQuery.extend (true, {}, data);
     }
  });
  setTimeout (update_system_stats, 2000);
}
update_system_stats();
"""

JS_CPU = """
function update_meter_cpu() {
  $('#%(bar_id)s').progressbar ('option', 'value', system_stats['cpu']['usage']);
  $('#%(details_id)s').html (system_stats['cpu']['usage'] + '&#37;');

  setTimeout (update_meter_cpu, 2000);
}
setTimeout (update_meter_cpu, 1000);
"""

JS_MEMORY = """
function update_meter_memory() {
  var used_str;
  var free_str;
  var free_amount  = (system_stats['mem']['total'] - system_stats['mem']['used']);
  var used_percent = system_stats['mem']['used'] / system_stats['mem']['total'] * 100;

  // 1048576 = 1024**2

  if (system_stats['mem']['total'] < 1048576) {
     used_str = (system_stats['mem']['used'] / 1024).toFixed(1) + "MB Used";
  } else {
     used_str = (system_stats['mem']['used'] / (1048576)).toFixed(1) + "GB Used";
  }

  if (free_amount < 1048576) {
     free_str = (free_amount / 1024).toFixed(1) + "MB Free";
  } else {
     free_str = (free_amount / (1048576)).toFixed(1) + "GB Free";
  }

  $('#%(bar_id)s').progressbar ('option', 'value', used_percent);
  $('#%(details_id)s').html (used_percent.toFixed(1) + '&#37;');
  $('#%(extra_id)s').html (used_str + ', '+ free_str);

  setTimeout (update_meter_memory, 2000);
}
setTimeout (update_meter_memory, 1000);
"""


class Meter (CTK.Box):
    def __init__ (self, name, extra_info_widget=None):
        CTK.Box.__init__ (self)
        self.progress = CTK.ProgressBar()
        self.details  = CTK.Box ({'class': 'progress-details'})
        self.extra    = CTK.Box ({'class': 'progress-extra'})

        box  = CTK.Box()
        if extra_info_widget:
            box += extra_info_widget
        box += self.extra
        self += box

        box  = CTK.Box()
        box += self.progress
        box += self.details
        self += box

    def Render (self, js):
        render = CTK.Box.Render (self)
        render.js += Uniq_Block (JS_COMMON)

        props = {'bar_id':     self.progress.id,
                 'details_id': self.details.id,
                 'extra_id':   self.extra.id }

        render.js += js %(props)
        return render


#
# CPU
#

class CPU_Info (CTK.RawHTML):
    def __init__ (self):
        stats = SystemStats.get_system_stats()

        parts = []
        if stats.cpu.speed:
            parts.append (stats.cpu.speed)

        if stats.cpu.num:
            if int(stats.cpu.num) > 1:
                parts.append (_("%s Logical Processors") %(stats.cpu.num))
            elif int(stats.cpu.num) == 1:
                parts.append (_("%s Logical Processor") %(stats.cpu.num))

        if stats.cpu.cores:
            if int(stats.cpu.cores) > 1:
                parts.append (_("%s Cores") %(stats.cpu.cores))
            elif int(stats.cpu.cores) == 1:
                parts.append (_("%s Core") %(stats.cpu.cores))

        if parts:
            txt = ', '.join(parts)
        else:
            txt = _('Unknown Processor')

        CTK.RawHTML.__init__ (self, txt)


class CPU_Meter (Meter):
    def __init__ (self):
        Meter.__init__ (self, 'cpu')

    def Render (self):
        return Meter.Render (self, JS_CPU)


#
# Memory
#

class Memory_Info (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'ram-text'})

        stats = SystemStats.get_system_stats()

        if stats.mem.total:
            if stats.mem.total < (1024**2):
                total = "%dMB" %(stats.mem.total / 1024)
            else:
                total = "%.1fGB" %(stats.mem.total / (1024.0 ** 2))
        else:
            total = _('Unknown RAM')

        self += CTK.RawHTML (total)


class Memory_Meter (Meter):
    def __init__ (self):
        Meter.__init__ (self, 'memory', Memory_Info())

    def Render (self):
        return Meter.Render (self, JS_MEMORY)


#
# /system/stats
#

def SystemStats_JSON():
    stats = SystemStats.get_system_stats()
    return {'mem': {'used':  stats.mem.used,
                    'total': stats.mem.total},
            'cpu': {'usage': stats.cpu.usage,
                    'idle':  stats.cpu.idle}}

CTK.publish (r'^/system/stats$', SystemStats_JSON)
