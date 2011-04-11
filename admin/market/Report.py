# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2011 Alvaro Lopez Ortega
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

import re
import os
import CTK
import OWS_Login
import SystemInfo

from consts import *
from ows_consts import *
from configured import *
from XMLServerDigest import XmlRpcServer

URL_REPORT_OK    = '%s/ok'    %(URL_REPORT)
URL_REPORT_FAIL  = '%s/fail'  %(URL_REPORT)
URL_REPORT_APPLY = '%s/apply' %(URL_REPORT)

NOTE_REPORT_H2      = N_('Contact Support & Customer Service')
NOTE_REPORT_EXPLAIN = N_('You are about to submit a report for an application that is not working for you.')
NOTE_REPORT_COMMENT = N_('Please, send your report to our support team. Some information <strong>will</strong> be attached upon submission for troubleshooting purposes. This includes your configuration file along with the installation logs for the application.')
NOTE_REPORT_ERROR   = N_('We are sorry, but the report could not be sent.')
NOTE_REPORT_TIPS    = N_('Make sure you are logged in and there are no connectivity issues. If the problem persists, please contact our support team.')
NOTE_REPORT_THANKS  = N_('Thank you for submitting the application report. Our support team will review it as soon as possible.')

NUM_LINES = 10

def get_logs (app_id):
    if not os.path.exists (CHEROKEE_OWS_ROOT) or \
       not os.access (CHEROKEE_OWS_ROOT, os.R_OK):
        return ['No CHEROKEE_OWS_ROOT.']

    logs = []
    for d in os.listdir (CHEROKEE_OWS_ROOT):
        fd = os.path.join (CHEROKEE_OWS_ROOT, d, 'install.log')
        try:
            head = open(fd, 'r').readlines(NUM_LINES)
            log  = ''.join (head)
            tmp  = re.findall ("Checking: (.+)?, ID: (%s)" %(app_id), log, re.M)
            if tmp:
                logs.append (log)
        except IOError, e:
            logs.append (str(e))
    return logs


def Report_Apply():
    app_id   = CTK.post.get_val('app_id')
    report   = CTK.post.get_val('report')
    app_logs = get_logs (app_id)
    sysinfo  = SystemInfo.get_info()
    cfg      = str(CTK.cfg)

    # OWS Open
    xmlrpc = XmlRpcServer(OWS_APPS_CENTER, OWS_Login.login_user, OWS_Login.login_password)
    try:
        ok = xmlrpc.report_application (app_id,                        # int
                                        CTK.util.to_unicode(report),   # string
                                        CTK.util.to_unicode(app_logs), # list
                                        CTK.util.to_unicode(sysinfo),  # dict
                                        CTK.util.to_unicode(cfg))      # string
    except:
        ok = False

    return {'ret': ('error','ok')[ok]}


class Report_Fail:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ("<h1>%s</h1>"%(_('Could not send the report')))
        cont += CTK.RawHTML ("<p>%s</p>"%(_(NOTE_REPORT_ERROR)))
        cont += CTK.RawHTML ("<p>%s</p>"%(_(NOTE_REPORT_TIPS)))

        buttons = CTK.DruidButtonsPanel()
        buttons += CTK.DruidButton_Close(_('Close'))
        cont += buttons

        return cont.Render().toStr()


class Report_Success:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ("<h1>%s</h1>"%(_('Report sent successfully')))
        cont += CTK.RawHTML ("<p>%s</p>"%(_(NOTE_REPORT_THANKS)))

        buttons = CTK.DruidButtonsPanel()
        buttons += CTK.DruidButton_Close(_('Close'))
        cont += buttons

        return cont.Render().toStr()


class Report:
    def __call__ (self):
        application_id = CTK.request.url.split('/')[-1]

        # Build the content
        submit = CTK.Submitter (URL_REPORT_APPLY)
        submit += CTK.TextArea ({'name': 'report', 'rows':10, 'cols': 60, 'class': 'noauto'})
        submit += CTK.Hidden ('app_id', application_id)
        submit.bind ('submit_fail', CTK.DruidContent__JS_to_goto (submit.id, URL_REPORT_FAIL))
        submit.bind ('submit_success', CTK.DruidContent__JS_to_goto (submit.id, URL_REPORT_OK))

        cont  = CTK.Container()
        cont += CTK.RawHTML ("<h2>%s</h2>"%(_(NOTE_REPORT_H2)))
        cont += CTK.RawHTML ("<p>%s</p>"%(_(NOTE_REPORT_EXPLAIN)))
        cont += CTK.RawHTML ("<p>%s</p>"%(_(NOTE_REPORT_COMMENT)))
        cont += submit

        buttons = CTK.DruidButtonsPanel()
        buttons += CTK.DruidButton_Submit (_("Report"))
        buttons += CTK.DruidButton_Close (_("Cancel"))
        cont += buttons

        return cont.Render().toStr()


CTK.publish ('^%s'%(URL_REPORT),        Report)
CTK.publish ('^%s$'%(URL_REPORT_FAIL),  Report_Fail)
CTK.publish ('^%s$'%(URL_REPORT_OK),    Report_Success)
CTK.publish ('^%s$'%(URL_REPORT_APPLY), Report_Apply, method="POST")
