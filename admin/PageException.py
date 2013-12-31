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

import os
import CTK
import Cherokee
import configured
from CTK.util import formatter

from urllib import quote, unquote, urlencode
from httplib import HTTPConnection

URL_BTS         = 'http://bugs.cherokee-project.com/new'
URL_REPORT_HOST = 'www.cherokee-project.com'
URL_REPORT_URL  = '/CTK_ok.html'

NOTE_EXCEPT_SORRY   = N_('We apologize for any inconveniences that this may have caused.')
NOTE_EXCEPT_COMMENT = N_('Please, send your comments to the server developers. Your configuration file <strong>will</strong> be attached upon submission. What were you doing when the issue showed up?')
NOTE_EXCEPT_THANKS  = N_('Thank you for reporting the problem. The Cherokee Web Server developers will try to fix it up as soon as possible.')
NOTE_EXCEPT_FAIL    = N_('<p>For some reason, the issue could not be reported to the Cherokee project.</p><p>Please, do not hesitate to <a target="_blank" href="%s">report the issue</a> on our bug tracking system if the problem persists.</p>') %(URL_BTS)

URL_APPLY  = '/exception/apply'


def submit():
    # Build the parameters
    headers = {"Content-type": "application/x-www-form-urlencoded"}
    info    = urlencode ({'version':   configured.VERSION,
                          'comments':  CTK.post['comments'],
                          'traceback': unquote (CTK.post['traceback']),
                          'config':    str(CTK.cfg)})

    # HTTP Request
    conn = HTTPConnection (URL_REPORT_HOST)
    conn.request ("POST", URL_REPORT_URL, info, headers)
    response = conn.getresponse()
    data = response.read()
    conn.close()

    # Check the output
    if response.status != 200:
        return {'ret': 'error'}
    if not "{'ret':'ok'}" in data:
        return {'ret': 'error'}

    return {'ret':'ok'}


class Page (CTK.Page):
    def __init__ (self, traceback, desc, **kwargs):
        # Look for the theme file
        srcdir = os.path.dirname (os.path.realpath (__file__))
        theme_file = os.path.join (srcdir, 'exception.html')
        bt = quote(traceback).replace ('%', '%%')

        # Set up the template
        template = CTK.Template (filename = theme_file)

        template['title']      = _("Internal Error")
        template['body_props'] = ' id="body-error"'

        # Parent's constructor
        CTK.Page.__init__ (self, template, **kwargs)

        # Thank you: submitted
        dialog_ok = CTK.Dialog ({'title': _("Thank you!"), 'autoOpen': False, 'draggable': False, 'width': 480})
        dialog_ok += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_EXCEPT_THANKS)))
        dialog_ok.AddButton (_('Close'), "close")

        dialog_fail = CTK.Dialog ({'title': _("Could not report the issue"), 'autoOpen': False, 'draggable': False, 'width': 480})
        dialog_fail += CTK.Notice ('error', CTK.RawHTML (_(NOTE_EXCEPT_FAIL)))
        dialog_fail.AddButton (_('Close'), "close")

        self += dialog_ok
        self += dialog_fail

        # Build the page content
        self += CTK.RawHTML ('<h1>%s</h1>' %(_("Unexpected Exception")))
        self += CTK.RawHTML ('<h2>%s</h2>' %(desc))
        self += CTK.RawHTML (_(NOTE_EXCEPT_SORRY))
        self += CTK.RawHTML (formatter ('<pre class="backtrace">%s</pre>', traceback))
        self += CTK.RawHTML (_(NOTE_EXCEPT_COMMENT))

        submit = CTK.Submitter (URL_APPLY)
        submit.bind ('submit_success', dialog_ok.JS_to_show())
        submit.bind ('submit_fail',    dialog_fail.JS_to_show())
        self += submit

        submit += CTK.TextArea ({'name': 'comments', 'rows':10, 'cols': 80, 'class': 'noauto'})
        submit += CTK.Hidden ('traceback', bt)
        submit += CTK.SubmitterButton (_("Report this"))


CTK.publish (URL_APPLY, submit, method="POST")
