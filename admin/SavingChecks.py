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
from CTK.i18n import _, N_
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
        for r in CTK.cfg[f'vserver!{v}!rule'] or []:
            handler_name  = CTK.cfg.get_val (f'vserver!{v}!rule!{r}!handler')
            balancer_name = CTK.cfg.get_val (f'vserver!{v}!rule!{r}!handler!balancer')

            if handler_name and \
               handler_name in consts.HANDLERS_WITH_BALANCER:
                srcs = CTK.cfg.keys (f'vserver!{v}!rule!{r}!handler!balancer!source')
                if not srcs:
                    errors.append (Error(_('Balancer without any source'),
                                         f'/vserver/{v}/rule/{r}#2'))

    #
    # Sources without interpreter
    #
    for s in CTK.cfg['source'] or []:
        sourcetype = CTK.cfg.get_val (f'source!{s}!type')
        interpreter = CTK.cfg.get_val (f'source!{s}!interpreter')
        chroot = CTK.cfg.get_val (f'source!{s}!chroot')

        if sourcetype == 'interpreter':
            if (not CTK.cfg.get_val (f'source!{s}!interpreter') or \
                interpreter.strip() == ''):
                errors.append (Error(_('Source without Interpreter'),
                                       f'/source/{s}'))

            if chroot:
                if chroot[0] != '/':
                    errors.append (Error(_('Chroot folder is not an absolute path'),
                                           f'/source/{s}'))
                else:
                    if not os.path.exists(chroot):
                        errors.append (Error(_('Chroot folder does not exist'),
                                               f'/source/{s}'))

                    elif not os.path.exists(chroot+interpreter.split(' ')[0]):
                        errors.append (Error(_('Interpreter does not exist inside chroot'),
                                               f'/source/{s}'))

            elif not os.path.exists(interpreter.split(' ')[0]):
                 errors.append (Error(_('Interpreter does not exist'),
                                        f'/source/{s}'))

    #
    # Validators without Realm
    #
    for v in CTK.cfg['vserver'] or []:
        for r in CTK.cfg[f'vserver!{v}!rule'] or []:
            if CTK.cfg.get_val (f'vserver!{v}!rule!{r}!auth'):
                if not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!auth!realm'):
                    errors.append (Error(_('Authentication rule without Realm'),
                                         f'/vserver/{v}/rule/{r}#5'))

    #
    # Virtual server without document_root, nick or no wildcard match
    #
    for v in CTK.cfg['vserver'] or []:
        if not CTK.cfg.get_val (f'vserver!{v}!document_root'):
            errors.append (Error(_('Virtual Server without document root'),
                                 f'/vserver/{v}#1'))

        if not CTK.cfg.get_val (f'vserver!{v}!nick'):
            errors.append (Error(_('Virtual Server without nickname'),
                                 f'/vserver/{v}#2'))

        if CTK.cfg.get_val (f'vserver!{v}!match') == 'wildcard':
            wildcards = CTK.cfg.keys (f'vserver!{v}!match!domain')
            if not wildcards:
                errors.append (Error(_('Virtual Server without wildcard string'),
                                     f'/vserver/{v}#2'))

    #
    # Broken rule matches
    #
    for v in CTK.cfg['vserver'] or []:
        for r in CTK.cfg[f'vserver!{v}!rule'] or []:
            match = CTK.cfg.get_val (f'vserver!{v}!rule!{r}!match')

            if match == 'directory' and \
               not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!match!directory'):
                errors.append (Error(_('Directory match without a directory'),
                                     f'/vserver/{v}/rule/{r}#1'))

            elif match == 'extensions' and \
                 not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!match!extensions'):
                errors.append (Error(_('Extensions match without any extension'),
                                     f'/vserver/{v}/rule/{r}#1'))

            elif match == 'request' and \
                 not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!match!request'):
                errors.append (Error(_('RegEx match without a RegEx entry'),
                                     f'/vserver/{v}/rule/{r}#1'))

            elif match == 'header':
                if not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!match!header'):
                    errors.append (Error(_('Header match without a defined header'),
                                         f'/vserver/{v}/rule/{r}#1'))
                if CTK.cfg.get_val (f'vserver!{v}!rule!{r}!match!type') == 'regex' and \
                   (not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!match!match') or \
                        CTK.cfg.get_val (f'vserver!{v}!rule!{r}!match!match').strip() == ''):
                    errors.append (Error(_('Header match without a matching expression'),
                                         f'/vserver/{v}/rule/{r}#1'))

            elif match == 'exists':
                any = int (CTK.cfg.get_val (f'vserver!{v}!rule!{r}!match!match_any', "0"))
                if not any and not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!match!exists'):
                    errors.append (Error(_("File exists rule without a file or 'Any file'"),
                                         f'/vserver/{v}/rule/{r}#1'))

            elif match == 'method' and \
                 not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!match!method'):
                errors.append (Error(_('Method match without a defined method'),
                                     f'/vserver/{v}/rule/{r}#1'))

            elif match == 'bind' and \
                 not CTK.cfg[f'vserver!{v}!rule!{r}!match!bind']:
                errors.append (Error(_("Empty 'Incoming IP/Port' match"),
                                     f'/vserver/{v}/rule/{r}#1'))

            elif match == 'fullpath' and \
                 not CTK.cfg[f'vserver!{v}!rule!{r}!match!fullpath']:
                errors.append (Error(_("Empty 'Full path' rule match"),
                                     f'/vserver/{v}/rule/{r}#1'))

            elif match == 'from' and \
                 not CTK.cfg[f'vserver!{v}!rule!{r}!match!from']:
                errors.append (Error(_("Empty 'Connected from' rule match"),
                                     f'/vserver/{v}/rule/{r}#1'))

            elif match == 'url_arg' and \
                (not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!match!arg') or \
                 not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!match!match')):
                errors.append (Error(_("Incomplete 'URL Argument' rule match"),
                                     f'/vserver/{v}/rule/{r}#1'))

            elif match == 'geoip' and \
                 not CTK.cfg[f'vserver!{v}!rule!{r}!match!countries']:
                errors.append (Error(_("GeoIP match rule with no countries"),
                                     f'/vserver/{v}/rule/{r}#1'))

    #
    # Incomplete regular expressions
    #
    for v in CTK.cfg['vserver'] or []:
        for r in CTK.cfg[f'vserver!{v}!rule'] or []:
            handler = CTK.cfg.get_val (f'vserver!{v}!rule!{r}!handler')

            if handler == 'proxy':
                for x in CTK.cfg[f'vserver!{v}!rule!{r}!handler!in_rewrite_request'] or []:
                    if not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!handler!in_rewrite_request!{x}!regex'):
                        errors.append (Error(_("Request URL Rewriting rule list has a missing Regular Expression"),
                                            f'/vserver/{v}/rule/{r}#2'))

                    if not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!handler!in_rewrite_request!{x}!substring'):
                        errors.append (Error(_("Request URL Rewriting rule list has a missing Substitution"),
                                            f'/vserver/{v}/rule/{r}#2'))

                for x in CTK.cfg[f'vserver!{v}!rule!{r}!handler!out_rewrite_request'] or []:
                    if not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!handler!out_rewrite_request!{x}!regex'):
                        errors.append (Error(_("Reply URL Rewriting rule list has a missing Regular Expression"),
                                            f'/vserver/{v}/rule/{r}#2'))

                    if not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!handler!out_rewrite_request!{x}!substring'):
                        errors.append (Error(_("Reply URL Rewriting rule list has a missing Substitution"),
                                            f'/vserver/{v}/rule/{r}#2'))

            elif handler == 'redir':
                for x in CTK.cfg[f'vserver!{v}!rule!{r}!handler!rewrite'] or []:
                    if not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!handler!rewrite!{x}!regex'):
                        errors.append (Error(_("Redirection rule list has a missing Regular Expression"),
                                            f'/vserver/{v}/rule/{r}#2'))

                    if not CTK.cfg.get_val (f'vserver!{v}!rule!{r}!handler!rewrite!{x}!substring'):
                        errors.append (Error(_("Redirection rule list has a missing Substitution"),
                                            f'/vserver/{v}/rule/{r}#2'))
    return errors
