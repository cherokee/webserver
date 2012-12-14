# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2013 Alvaro Lopez Ortega
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
import consts

class Error:
    def __init__ (self, title='', url=''):
        self.title = title[:]
        self.url   = url[:]


def check_config():
    errors = []

    #
    # Empty balancers
    #
    for v in CTK.cfg['vserver'] or []:
        for r in CTK.cfg['vserver!%s!rule'%(v)] or []:
            handler_name  = CTK.cfg.get_val ('vserver!%s!rule!%s!handler'%(v,r))
            balancer_name = CTK.cfg.get_val ('vserver!%s!rule!%s!handler!balancer'%(v,r))

            if handler_name and \
               handler_name in consts.HANDLERS_WITH_BALANCER:
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

    #
    # Virtual server without document_root or nick
    #
    for v in CTK.cfg['vserver'] or []:
        if not CTK.cfg.get_val ('vserver!%s!nick'%(v)):
            errors.append (Error(_('Virtual Server without nickname'), '/vserver'))

        if not CTK.cfg.get_val ('vserver!%s!document_root'%(v)):
            errors.append (Error(_('Virtual Server without document root'), '/vserver'))

    #
    # Broken rule matches
    #
    for v in CTK.cfg['vserver'] or []:
        for r in CTK.cfg['vserver!%s!rule'%(v)] or []:
            match = CTK.cfg.get_val ('vserver!%s!rule!%s!match'%(v, r))

            if match == 'directory' and \
               not CTK.cfg.get_val ('vserver!%s!rule!%s!match!directory'%(v,r)):
                errors.append (Error(_('Directory match without a directory'),
                                     '/vserver/%s/rule/%s#1'%(v,r)))

            elif match == 'extensions' and \
                 not CTK.cfg.get_val ('vserver!%s!rule!%s!match!extensions'%(v,r)):
                errors.append (Error(_('Extensions match without any extension'),
                                     '/vserver/%s/rule/%s#1'%(v,r)))

            elif match == 'request' and \
                 not CTK.cfg.get_val ('vserver!%s!rule!%s!match!request'%(v,r)):
                errors.append (Error(_('RegEx match without a RegEx entry'),
                                     '/vserver/%s/rule/%s#1'%(v,r)))

            elif match == 'header':
                if not CTK.cfg.get_val ('vserver!%s!rule!%s!match!header'%(v,r)):
                    errors.append (Error(_('Header match without a defined header'),
                                         '/vserver/%s/rule/%s#1'%(v,r)))
                if not CTK.cfg.get_val ('vserver!%s!rule!%s!match!match'%(v,r)):
                    errors.append (Error(_('Header match without a matching expression'),
                                         '/vserver/%s/rule/%s#1'%(v,r)))

            elif match == 'exists':
                any = int (CTK.cfg.get_val ('vserver!%s!rule!%s!match!match_any'%(v,r), "0"))
                if not any and not CTK.cfg.get_val ('vserver!%s!rule!%s!match!exists'%(v,r)):
                    errors.append (Error(_("File exists rule without a file or 'Any file'"),
                                         '/vserver/%s/rule/%s#1'%(v,r)))

            elif match == 'method' and \
                 not CTK.cfg.get_val ('vserver!%s!rule!%s!match!method'%(v,r)):
                errors.append (Error(_('Method match without a defined method'),
                                     '/vserver/%s/rule/%s#1'%(v,r)))

            elif match == 'bind' and \
                 not CTK.cfg['vserver!%s!rule!%s!match!bind'%(v,r)]:
                errors.append (Error(_("Empty 'Incoming IP/Port' match"),
                                     '/vserver/%s/rule/%s#1'%(v,r)))

            elif match == 'fullpath' and \
                 not CTK.cfg['vserver!%s!rule!%s!match!fullpath'%(v,r)]:
                errors.append (Error(_("Empty 'Full path' rule match"),
                                     '/vserver/%s/rule/%s#1'%(v,r)))

            elif match == 'from' and \
                 not CTK.cfg['vserver!%s!rule!%s!match!from'%(v,r)]:
                errors.append (Error(_("Empty 'Connected from' rule match"),
                                     '/vserver/%s/rule/%s#1'%(v,r)))

            elif match == 'url_arg' and \
                (not CTK.cfg.get_val ('vserver!%s!rule!%s!match!arg'%(v,r)) or \
                 not CTK.cfg.get_val ('vserver!%s!rule!%s!match!match'%(v,r))):
                errors.append (Error(_("Incomplete 'URL Argument' rule match"),
                                     '/vserver/%s/rule/%s#1'%(v,r)))

            elif match == 'geoip' and \
                 not CTK.cfg['vserver!%s!rule!%s!match!countries'%(v,r)]:
                errors.append (Error(_("GeoIP match rule with no countries"),
                                     '/vserver/%s/rule/%s#1'%(v,r)))

    return errors
