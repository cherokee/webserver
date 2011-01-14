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

import CTK

class Error:
    def __init__ (self, title='', url=''):
        self.title = title[:]
        self.url   = url[:]


def does_handler_use_balancer (handler_name):
    return handler_name in ('fcgi', 'scgi', 'uwsgi', 'proxy')


def check_config():
    errors = []

    #
    # Empty balancers
    #
    for v in CTK.cfg['vserver'] or []:
        for r in CTK.cfg['vserver!%s!rule'%(v)] or []:
            handler_name  = CTK.cfg.get_val ('vserver!%s!rule!%s!handler'%(v,r))
            balancer_name = CTK.cfg.get_val ('vserver!%s!rule!%s!handler!balancer'%(v,r))

            if does_handler_use_balancer (handler_name) or balancer_name:
                srcs = CTK.cfg.keys ('vserver!%s!rule!%s!handler!balancer!source'%(v,r))
                if not srcs:
                    errors.append (Error(_('Balancer without any source'),
                                         '/vserver/%s/rule/%s#2'%(v,r)))

    #
    # Validators without Realm
    #
    for v in CTK.cfg['vserver'] or []:
        for r in CTK.cfg['vserver!%s!rule'%(v)] or []:
            if CTK.cfg.get_val ('vserver!%s!rule!%s!auth'%(v,r)):
                if not CTK.cfg.get_val ('vserver!%s!rule!%s!auth!realm'%(v,r)):
                    errors.append (Error(_('Authentication rule without Realm'),
                                         '/vserver/%s/rule/%s#5'%(v,r)))

    return errors
