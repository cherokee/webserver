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
import consts
import os

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
    # Sources without interpreter
    #
    for s in CTK.cfg['source'] or []:
        sourcetype = CTK.cfg.get_val ('source!%s!type'%(s))
        interpreter = CTK.cfg.get_val ('source!%s!interpreter'%(s))
        chroot = CTK.cfg.get_val ('source!%s!chroot'%(s))

        if sourcetype == 'interpreter':
            if (not CTK.cfg.get_val ('source!%s!interpreter'%(s)) or \
                interpreter.strip() == ''):
                errors.append (Error(_('Source without Interpreter'),
                                       '/source/%s'%(s)))

            if chroot:
                if chroot[0] != '/':
                    errors.append (Error(_('Chroot folder is not an absolute path'),
                                           '/source/%s'%(s)))
                else:
                    if not os.path.exists(chroot):
                        errors.append (Error(_('Chroot folder does not exist'),
                                               '/source/%s'%(s)))

                    elif not os.path.exists(chroot+interpreter.split(' ')[0]):
                        errors.append (Error(_('Interpreter does not exist inside chroot'),
                                               '/source/%s'%(s)))

            elif not os.path.exists(interpreter.split(' ')[0]):
                 errors.append (Error(_('Interpreter does not exist'),
                                        '/source/%s'%(s)))

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
    # Virtual server without document_root, nick or no wildcard match
    #
    for v in CTK.cfg['vserver'] or []:
        if not CTK.cfg.get_val ('vserver!%s!document_root'%(v)):
            errors.append (Error(_('Virtual Server without document root'),
                                 '/vserver/%s#1'%(v)))

        if not CTK.cfg.get_val ('vserver!%s!nick'%(v)):
            errors.append (Error(_('Virtual Server without nickname'),
                                 '/vserver/%s#2'%(v)))

        if CTK.cfg.get_val ('vserver!%s!match'%(v)) == 'wildcard':
            wildcards = CTK.cfg.keys ('vserver!%s!match!domain'%(v))
            if not wildcards:
                errors.append (Error(_('Virtual Server without wildcard string'),
                                     '/vserver/%s#2'%(v)))

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
                if CTK.cfg.get_val ('vserver!%s!rule!%s!match!type'%(v,r)) == 'regex' and \
                   (not CTK.cfg.get_val ('vserver!%s!rule!%s!match!match'%(v,r)) or \
                        CTK.cfg.get_val ('vserver!%s!rule!%s!match!match'%(v,r)).strip() == ''):
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

    #
    # Incomplete regular expressions
    #
    for v in CTK.cfg['vserver'] or []:
        for r in CTK.cfg['vserver!%s!rule'%(v)] or []:
            handler = CTK.cfg.get_val ('vserver!%s!rule!%s!handler'%(v,r))

            if handler == 'proxy':
                for x in CTK.cfg['vserver!%s!rule!%s!handler!in_rewrite_request'%(v,r)] or []:
                    if not CTK.cfg.get_val ('vserver!%s!rule!%s!handler!in_rewrite_request!%s!regex'%(v,r,x)):
                        errors.append (Error(_("Request URL Rewriting rule list has a missing Regular Expression"),
                                            '/vserver/%s/rule/%s#2'%(v,r)))

                    if not CTK.cfg.get_val ('vserver!%s!rule!%s!handler!in_rewrite_request!%s!substring'%(v,r,x)):
                        errors.append (Error(_("Request URL Rewriting rule list has a missing Substitution"),
                                            '/vserver/%s/rule/%s#2'%(v,r)))

                for x in CTK.cfg['vserver!%s!rule!%s!handler!out_rewrite_request'%(v,r)] or []:
                    if not CTK.cfg.get_val ('vserver!%s!rule!%s!handler!out_rewrite_request!%s!regex'%(v,r,x)):
                        errors.append (Error(_("Reply URL Rewriting rule list has a missing Regular Expression"),
                                            '/vserver/%s/rule/%s#2'%(v,r)))

                    if not CTK.cfg.get_val ('vserver!%s!rule!%s!handler!out_rewrite_request!%s!substring'%(v,r,x)):
                        errors.append (Error(_("Reply URL Rewriting rule list has a missing Substitution"),
                                            '/vserver/%s/rule/%s#2'%(v,r)))

            elif handler == 'redir':
                for x in CTK.cfg['vserver!%s!rule!%s!handler!rewrite'%(v,r)] or []:
                    if not CTK.cfg.get_val ('vserver!%s!rule!%s!handler!rewrite!%s!regex'%(v,r,x)):
                        errors.append (Error(_("Redirection rule list has a missing Regular Expression"),
                                            '/vserver/%s/rule/%s#2'%(v,r)))

                    if not CTK.cfg.get_val ('vserver!%s!rule!%s!handler!rewrite!%s!substring'%(v,r,x)):
                        errors.append (Error(_("Redirection rule list has a missing Substitution"),
                                            '/vserver/%s/rule/%s#2'%(v,r)))
    return errors
