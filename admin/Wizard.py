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

import re
import os
import CTK
import copy

URL_CAT_LIST_VSRV   =   '/wizard/category/vsrv'
URL_CAT_LIST_VSRV_R = r'^/wizard/category/vsrv/(.*)$'
URL_CAT_LIST_RULE   =   '/wizard/category/rule'
URL_CAT_LIST_RULE_R = r'^/wizard/category/rule/(.*)$'

URL_CAT_APPLY  = '/wizard/new/apply'

TYPE_VSERVER = 1
TYPE_RULE    = 1 << 2

USUAL_STATIC_FILES = ['/favicon.ico', '/robots.txt', '/crossdomain.xml',
                      '/sitemap.xml', '/sitemap.xml.gz']

DEFAULT_RECIPE_TEXT= N_('Check the cookbook for more details.')

class Wizard (CTK.Box):
    def __init__ (self, _props={}):
        props = _props.copy()

        if 'class' in self.props:
            props['class'] += ' wizard'
        else:
            props['class'] = 'wizard'

        CTK.Box.__init__ (self, props)

    def show (self):
        raise NotImplementedError, 'Wizard.show must be implemented'


class Wizard_Rule (CTK.Box):
    def __init__ (self, key):
        Wizard.__init__ (self)
        self.key = key


class WizardList (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'wizard-list'})


def filter_wizard_list (w_list, filter):
    ret_list = copy.deepcopy(w_list)

    # Remove wizards
    for group in ret_list:
        wizards = group['list']

        to_del = []
        for wizard in wizards:
            if not wizard['type'] & filter:
                to_del.append (wizard)

        for wizard in to_del:
            del (wizards[wizards.index(wizard)])

    # Remove empty groups
    to_del = []
    for group in ret_list:
        if not group['list']:
            to_del.append (group)

    for group in to_del:
        del (ret_list[ret_list.index(group)])

    return ret_list


class Categories:
    def __init__ (self, wizards_type):
        wizards = CTK.load_module ('List', 'wizards')
        self.list = filter_wizard_list (wizards.LIST, wizards_type)

    def __iter__ (self):
        return iter(self.list)

class Icon (CTK.Image):
    def __init__ (self, name, _props={}):
        props = _props.copy()
        props['src'] = '/static/images/wizards/%s.png'%(name)
        props['alt'] = "%s logo" %(name.capitalize())

        if 'class' in props:
            props['class'] += ' wizard-icon'
        else:
            props['class'] = 'wizard-icon'

        CTK.Image.__init__ (self, props)


# Handle 'click' events inside the Wizard Categories list.
#
JS_WIZARD_LIST = """
$('#%(list_id)s').each(function() {
    var box    = $(this);
    var hidden = box.find('input:hidden');

    box.find('li').each(function() {
        var li = $(this);

        li.bind ('click', function(event) {
            box.find('li').removeClass('wizard-list-selected');
            $(this).addClass ('wizard-list-selected');
            hidden.val (li.attr('wizard'));
        });
    });
});
"""

# Generates a 'open_wizard' event whenever the dialog is submitted.
#
JS_WIZARD_LIST_SUBMIT = """
var selected = $('#%(hidden)s').val();
if (selected) {
    $(this).trigger ({type: 'open_wizard', 'wizard': selected});
}

return false;
"""

class CategoryList_Widget (CTK.Box):
    def __init__ (self, category, wizards_type):
        CTK.Box.__init__ (self)
        self.category = category

        # Retrieve the list
        wizards = CTK.load_module ('List', 'wizards')
        wizard_list = filter_wizard_list (wizards.LIST, wizards_type)

        # Build the widgets list
        wlist = CTK.List({'class': 'wizard-list'})

        # Find the right group
        tmp = filter (lambda g: g['name'] == category, wizard_list)
        if tmp:
            for w in tmp[0]['list']:
                descr, title = w['descr'], w['title']
                wlist.Add ([CTK.Box({'class': 'logo'},  Icon(w['name'])),
                            CTK.Box({'class': 'title'}, CTK.RawHTML(_(title))),
                            CTK.Box({'class': 'descr'}, CTK.RawHTML(_(descr)))],
                           {'wizard': w['name']})

        hidden = CTK.Hidden ('wizard')

        # Assembling
        submit = CTK.Submitter (URL_CAT_APPLY)
        submit += wlist
        submit += hidden
        submit += CTK.RawHTML (js = JS_WIZARD_LIST %({'list_id': self.id}))
        submit.bind('submit_success', JS_WIZARD_LIST_SUBMIT %({'hidden': hidden.id}))
        self += submit


def CategoryList_Vsrv():
    # Figure the category
    category = re.findall (URL_CAT_LIST_VSRV_R, CTK.request.url)[0]

    # Instance and Render
    content = CategoryList_Widget (category, TYPE_VSERVER)
    return content.Render().toJSON()

def CategoryList_Rule():
    # Figure the category
    category = re.findall (URL_CAT_LIST_RULE_R, CTK.request.url)[0]

    # Instance and Render
    content = CategoryList_Widget (category, TYPE_RULE)
    return content.Render().toJSON()


def CategoryList_Apply():
    return CTK.cfg_reply_ajax_ok()


#
# Widgets
#
class CloneLogsCfg (CTK.ComboCfg):
    NOTE = N_("Use the same logging configuration as one of the other virtual servers.")

    def __init__ (self, cfg):
        # Build the options list
        options = [('', _('Do not configure'))]

        for v in CTK.cfg.keys('vserver'):
            pre  = 'vserver!%s!logger' %(v)
            log  = CTK.cfg.get_val(pre)
            nick = CTK.cfg.get_val('vserver!%s!nick'%(v))
            if not log:
                continue
            options.append (('vserver!%s'%(v), '%s (%s)'%(nick, log)))

        # Init
        CTK.ComboCfg.__init__ (self, cfg, options, {'class': 'noauto'})


def CloneLogsCfg_Apply (key_tmp, vserver):
    # Check the source
    logging_as = CTK.cfg.get_val (key_tmp)
    if not logging_as:
        return

    # Clone
    CTK.cfg.clone ('%s!logger'%(logging_as),       '%s!logger'%(vserver))
    CTK.cfg.clone ('%s!error_writer'%(logging_as), '%s!error_writer'%(vserver))


def AddUsualStaticFiles (rule_pre, files = USUAL_STATIC_FILES):
    CTK.cfg['%s!match'%(rule_pre)]           = 'fullpath'
    CTK.cfg['%s!handler'%(rule_pre)]         = 'file'
    CTK.cfg['%s!handler!iocache'%(rule_pre)] = '1'
    CTK.cfg['%s!encoder!gzip'%(rule_pre)]    = '0'
    CTK.cfg['%s!encoder!deflate'%(rule_pre)] = '0'
    CTK.cfg['%s!expiration'%(rule_pre)]      = 'time'
    CTK.cfg['%s!expiration!time'%(rule_pre)] = '1h'

    n = 1
    for file in files:
        CTK.cfg['%s!match!fullpath!%d'%(rule_pre,n)] = file
        n += 1


class CookBookBox (CTK.Box):
    """References the local documentation"""
    def __init__ (self, help, text = DEFAULT_RECIPE_TEXT):
        CTK.Box.__init__(self, {'class': 'wizard-cookbook'})

        if help.startswith('http://'):
            link = help
        else:
            link = '/help/%s.html'%(help)

        self += CTK.Link (link, CTK.RawHTML(_(text)))


#
# Init (TEMPORARY)
#
def init():
    global _is_init
    _is_init = True

    wizards = [x[:-3] for x in os.listdir('wizards') if x.endswith('.py')]
    for w in wizards:
        CTK.load_module (w, 'wizards')

_is_init = False
if not _is_init:
    init()


CTK.publish (URL_CAT_LIST_VSRV_R,   CategoryList_Vsrv)
CTK.publish (URL_CAT_LIST_RULE_R,   CategoryList_Rule)
CTK.publish ('^%s'%(URL_CAT_APPLY), CategoryList_Apply, method="POST")
