# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#      Taher Shihadeh <taher@unixwars.com>
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
import Page
import Cherokee
import validations
import util

from consts import *
from configured import *

URL_APPLY = '/mime/apply'

NOTE_NEW_MIME       = N_('New MIME type to be added.')
NOTE_NEW_EXTENSIONS = N_('Comma separated list of file extensions associated with the MIME type.')
NOTE_NEW_MAXAGE     = N_('Maximum time that this sort of content can be cached (in seconds).')


VALIDATIONS = [
    ('new_mime', validations.is_safe_mime_type),
    ('new_exts', validations.is_safe_mime_exts),
    ('new_mage', validations.is_positive_int)
]

def commit():
    # New entry
    mime = CTK.post.pop('new_mime')
    exts = CTK.post.pop('new_exts')
    mage = CTK.post.pop('new_mage')

    if mime:
        if mage:
            CTK.cfg['mime!%s!max-age'%(mime)] = mage
        if exts:
            CTK.cfg['mime!%s!extensions'%(mime)] = exts

        return CTK.cfg_reply_ajax_ok()

    # Modifications
    updates = {}
    for k in CTK.post:
        if k.endswith('!extensions'):
            new = CTK.post[k]
            try:
                val = validations.is_safe_mime_exts (new, CTK.cfg.get_val(k))
                if util.lists_differ (val, new):
                    updates[k] = val
            except ValueError, e:
                return { "ret": "error", "errors": { k: str(e) }}
        elif k.endswith('!max-age'):
            new = CTK.post[k]
            if new:
                try:
                    val = validations.is_positive_int (new)
                    if val != new:
                        updates[k] = val
                except ValueError, e:
                    return { "ret": "error", "errors": { k: str(e) }}
        CTK.cfg[k] = CTK.post[k]

    if updates:
        return {'ret': 'unsatisfactory', 'updates': updates}

    return CTK.cfg_reply_ajax_ok()


class AddMime (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        table = CTK.PropsTable()
        table.Add (_('Mime Type'),  CTK.TextField({'name': 'new_mime', 'class': 'noauto'}), _(NOTE_NEW_MIME))
        table.Add (_('Extensions'), CTK.TextField({'name': 'new_exts', 'class': 'noauto optional'}), _(NOTE_NEW_EXTENSIONS))
        table.Add (_('Max Age'),    CTK.TextField({'name': 'new_mage', 'class': 'noauto optional'}), _(NOTE_NEW_MAXAGE))

        submit = CTK.Submitter(URL_APPLY)
        submit += table
        self += submit


class AddNew_Button (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'mime-button'})

        # Add New
        dialog = CTK.Dialog ({'title': _('Add New MIME-Type'), 'width': 550})
        dialog.AddButton (_('Cancel'), "close")
        dialog.AddButton (_('Add'), dialog.JS_to_trigger('submit'))
        dialog += AddMime()

        button = CTK.Button(_('Add New'))
        button.bind ('click', dialog.JS_to_show())
        dialog.bind ('submit_success', dialog.JS_to_close())
        dialog.bind ('submit_success', self.JS_to_trigger('submit_success'));

        self += button
        self += dialog


class MIME_Table (CTK.Container):
    def __init__ (self, refreshable, **kwargs):
        CTK.Container.__init__ (self, **kwargs)
        self += CTK.RawHTML ('<h2>%s</h2>' %_('Mime types'))

        # List
        headerbox = CTK.Box ({'class': "mimeheader"})
        headerbox += CTK.Box({'id': 'mh-type'}, CTK.RawHTML(_('MIME type')))
        headerbox += CTK.Box({'id': 'mh-ext'}, CTK.RawHTML(_('Extensions')))
        headerbox += CTK.Box({'id': 'mh-age'}, CTK.RawHTML(_('MaxAge (<em>secs</em>)')))
        headerbox += CTK.Box({'id': 'mh-del'})

        self += headerbox

        mimes = CTK.cfg.keys('mime')
        mimes.sort()

        pag = CTK.Paginator ('mimetable-content')
        for mime in mimes:
            pre = "mime!%s"%(mime)

            rm = CTK.ImageStock('del')
            rm.bind ('click', CTK.JS.Ajax (URL_APPLY, data = {pre: ''},
                                           complete = refreshable.JS_to_refresh()))

            box  = CTK.Box ({'class': 'mimeentry'})
            box += CTK.Box ({'class': 'md md-type'}, CTK.RawHTML('<span>%s</span>' %(mime)))
            box += CTK.Box ({'class': 'md md-ext'},  CTK.TextCfg ('%s!extensions'%(pre), False, {'size': 35}))
            box += CTK.Box ({'class': 'md md-age'},  CTK.TextCfg ('%s!max-age'%(pre),    True,  {'size': 6, 'maxlength': 6}))
            box += CTK.Box ({'class': 'md md-del'},  rm)

            #box += CTK.RawHTML ('%s' %(mime))
            #box += CTK.TextCfg ('%s!extensions'%(pre), False, {'size': 35})
            #box += CTK.TextCfg ('%s!max-age'%(pre),    True,  {'size': 6, 'maxlength': 6})
            #box += rm

            submit  = CTK.Submitter (URL_APPLY)
            submit += box

            pag += submit

        # Add New
        button = AddNew_Button()
        button.bind ('submit_success', refreshable.JS_to_refresh ())

        self += pag
        self += button

class MIME_Table_Instancer (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        # Refresher
        refresh = CTK.Refreshable ({'id': 'mime_table'})
        refresh.register (lambda: MIME_Table(refresh).Render())
        self += refresh


class MIME_Widget (CTK.Box):
    def __init__ (self, **kwargs):
        CTK.Box.__init__ (self, {'class':'mime_types'}, **kwargs)
        self += MIME_Table_Instancer()


CTK.publish ('^%s'%(URL_APPLY), commit, validation=VALIDATIONS, method="POST")
