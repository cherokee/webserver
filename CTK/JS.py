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

def Ajax (url, data='', type='POST', async=True, dataType='json',
          success=None, error=None, complete=None):
    if not success:
        success=''

    async_s = ['false','true'][async]
    js = "$.ajax ({type: '%(type)s', url: '%(url)s', async: %(async_s)s" %(locals())

    if data:
        js += ", data: %(data)s, dataType: '%(dataType)s'" %(locals())

    js += """, success: function (data) {
         %(success)s;

  	  /* Modified: Save button */
	  var modified     = data['modified'];
	  var not_modified = data['not-modified'];

	  if (modified != undefined) {
	    $(modified).show();
	    $(modified).removeClass('saved');
	  } else if (not_modified) {
	    $(not_modified).addClass('saved');
	  }
        }""" %(locals())
    if error:
        js += ", error: function() { %(error)s }" %(locals())
    if complete:
        js += ", complete: function() { %(complete)s }" %(locals())

    js += "});"
    return js

def Hide (id):
    return "$('#%s').hide();"%(id)
