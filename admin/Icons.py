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
import os
import validations
import util

from consts import *
from configured import *

URL_APPLY = '/icons/apply'

ICON_ROW_SIZE = 9

VALIDATIONS = [
    ('new_exts', validations.is_safe_icons_suffix),
    ('new_file', validations.is_safe_icons_file),
]

def modify():
    updates = {}
    for k in CTK.post:
        # Presses 'submit' without changing anything
        if k in ('icons!default', 'icons!directory', 'icons!parent_directory'):
            if not CTK.post.get_val(k):
                return {'ret': "error"}

        # Validations for files and suffixes
        validator = None
        if k.startswith('icons!suffix') and CTK.post[k]:
            validator = validations.is_safe_icons_suffix
        elif k.startswith ('icons!file') and CTK.post[k]:
            validator = validations.is_safe_icons_file

        if validator:
            new = CTK.post[k]
            try:
                val = validator (new, CTK.cfg.get_val(k))
                if util.lists_differ (val, new):
                    updates[k] = val
            except ValueError, e:
                return {'ret': "error", 'errors': {k: str(e)}}

    if updates:
        return {'ret': 'unsatisfactory', 'updates': updates}


def commit():
    # New extension
    new_exts = CTK.post.pop('new_exts')
    if new_exts:
        icon = CTK.post.get_val('new_exts_icon')
        if not icon:
            return CTK.cfg_reply_ajax_ok()

        CTK.cfg['icons!suffix!%s'%(icon)] = new_exts
        return CTK.cfg_reply_ajax_ok()

    # New file
    new_file = CTK.post.pop('new_file')
    if new_file:
        icon = CTK.post.get_val('new_file_icon')
        if not icon:
            return CTK.cfg_reply_ajax_ok()

        CTK.cfg['icons!file!%s'%(icon)] = new_file
        return CTK.cfg_reply_ajax_ok()

    # Modifications
    updates = modify()
    if updates:
        return updates

    return CTK.cfg_apply_post()


def prettyfier (filen):
    # Remove extension
    if '.' in filen:
        filen = filen[:filen.rindex('.')]

    # Remove underscores
    filen = filen.replace('_',' ').replace('.',' ')

    # Capitalize
    return filen.capitalize()


class ExtensionsTable (CTK.Container):
    def __init__ (self, refreshable, **kwargs):
        CTK.Container.__init__ (self, **kwargs)

        self += CTK.RawHTML ("<h2>%s</h2>" %_('Extension List'))

        # List
        icons = CTK.cfg.keys('icons!suffix')

        if not icons:
            button_class = 'no_extensions'
        else:
            button_class = 'extensions'
            table = CTK.Table()
            table.id = "icon_extensions"
            table += [None, CTK.RawHTML(_('Extensions'))]
            table.set_header(1)

            icons.sort()
            for k in icons:
                pre    = 'icons!suffix!%s'%(k)
                desc   = prettyfier(k)
                image  = CTK.Image ({'alt'  : desc, 'title': desc, 'src': os.path.join('/icons_local', k)})
                delete = CTK.ImageStock('del')
                submit = CTK.Submitter (URL_APPLY)
                submit += CTK.TextCfg (pre, props={'size': '46'})
                table += [image, submit, delete]

                delete.bind('click', CTK.JS.Ajax (URL_APPLY, data = {pre: ''},
                                                  complete = refreshable.JS_to_refresh()))

            self += CTK.Indenter (table)

        # Add New
        button = AdditionDialogButton ('new_exts', _('Add New Extension'), klass = button_class)
        button.bind ('submit_success', refreshable.JS_to_refresh())
        self += button


class FilesTable (CTK.Container):
    def __init__ (self, refreshable, **kwargs):
        CTK.Container.__init__ (self, **kwargs)

        self += CTK.RawHTML ("<h2>%s</h2>" %_('File Matches'))

        # List
        icons = CTK.cfg.keys('icons!file')

        if not icons:
            button_class = 'no_file'
        else:
            button_class = 'file'
            table = CTK.Table()
            table.id = "icon_files"
            table += [None, CTK.RawHTML(_('Files'))]
            table.set_header(1)

            for k in icons:
                pre     = 'icons!file!%s'%(k)
                desc    = prettyfier(k)
                image   = CTK.Image ({'alt'  : desc, 'title': desc, 'src': os.path.join('/icons_local', k)})
                submit  = CTK.Submitter (URL_APPLY)
                submit += CTK.TextCfg (pre, props={'size': '46'})
                delete  = CTK.ImageStock('del')
                table  += [image, submit, delete]

                delete.bind('click', CTK.JS.Ajax (URL_APPLY, data = {pre: ''},
                                                  complete = refreshable.JS_to_refresh()))

            self += CTK.Indenter (table)

        # Add New
        button = AdditionDialogButton ('new_file', _('Add New File'), klass = button_class)
        button.bind ('submit_success', refreshable.JS_to_refresh())
        self += button


class SpecialTable (CTK.Container):
    def __init__ (self, refreshable, **kwargs):
        CTK.Container.__init__ (self, **kwargs)

        table = CTK.Table()

        for k, desc in [('default',          _('Default')),
                        ('directory',        _('Directory')),
                        ('parent_directory', _('Go to Parent'))]:

            icon = CTK.cfg.get_val('icons!%s'%(k), 'blank.png')
            props = {'alt'  : desc,
                     'title': desc,
                     'src'  : '/icons_local/%s'%(icon)}
            image = CTK.Image (props)
            modify = _('Modify')
            button = AdditionDialogButton (k, "%(modify)s '%(desc)s'"%(locals()), selected = icon, submit_label = modify)
            button.bind ('submit_success', refreshable.JS_to_refresh())
            table += [image, CTK.RawHTML(desc), button]

        submit  = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML ("<h2>%s</h2>" %_('Special Files'))
        self += CTK.Indenter (submit)


class SpecialWidget_Instancer (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        # Refresher
        refresh = CTK.Refreshable ({'id': 'icons_special'})
        refresh.register (lambda: SpecialTable(refresh).Render())
        self += refresh


class ExtensionsWidget_Instancer (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        # Refresher
        refresh = CTK.Refreshable ({'id': 'icons_extensions'})
        refresh.register (lambda: ExtensionsTable(refresh).Render())
        self += refresh


class FilesWidget_Instancer (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        # Refresher
        refresh = CTK.Refreshable ({'id': 'icons_files'})
        refresh.register (lambda: FilesTable(refresh).Render())
        self += refresh


ICON_CHOOSER_JS = """
$('#%(iconchooserbox_id)s').each(function() {
  var chooser = $(this);
  var hidden  = chooser.find('input:hidden');

  chooser.find('img.icon_chooser_entry').each(function() {
    $(this).bind('click', function(event) {
         var image = $(this);
         hidden.val(image.attr('file'));
         chooser.find('.icon_chooser_entry').removeClass("icon_chooser_selected");
         image.addClass("icon_chooser_selected");
    });
  });
});
"""

class IconChooser (CTK.Box):
    def __init__ (self, field_name, prefix, **kwargs):
        CTK.Box.__init__ (self, {'class': 'icon-chooser-box'})
        selected = kwargs.get('selected')

        # Get the file list
        files, unused = self.get_files (prefix)
        total = len(files)

        # Build the table
        row   = []
        table = CTK.Table()
        for x in range(total):
            filename = files[x]
            desc     = prettyfier (filename)

            props = {'alt'  : desc,
                     'title': desc,
                     'file' : filename,
                     'src'  : '/icons_local/%s'%(filename),
                     'class': 'icon_chooser_entry icon_chooser_used'}

            if filename in unused:
                props['class'] = 'icon_chooser_entry icon_chooser_unused'
            if filename == selected:
                props['class'] = 'icon_chooser_entry icon_chooser_selected'

            image = CTK.Image(props)
            row.append (image)
            if (x+1) % ICON_ROW_SIZE == 0 or (x+1) == total:
                table += row
                row = []

        hidden = CTK.Hidden(field_name, '')
        self += hidden
        self += table

    def Render (self):
        render = CTK.Box.Render (self)
        render.js += ICON_CHOOSER_JS %({'iconchooserbox_id': self.id})
        return render


    def get_files (self, prefix = None):
        listdir = os.listdir (CHEROKEE_ICONSDIR)
        files   = []
        for filename in listdir:
            ext = filename.lower().split('.')[-1]
            if ext in ('jpg', 'png', 'gif', 'svg'):
                files.append(filename)

        if not prefix:
            return (files, files)

        icons   = CTK.cfg.keys(prefix)
        unused  = list(set(files) - set(icons))
        unused.sort()
        return (files, unused)


class AddIcon (CTK.Container):
    def __init__ (self, key, **kwargs):
        CTK.Container.__init__ (self)
        descs = {'new_file':         _('File'),
                 'new_exts':         _('Extensions'),
                 'default':          _('Default'),
                 'directory':        _('Directory'),
                 'parent_directory': _('Go to Parent')}
        assert key in descs.keys()
        label = descs[key]

        table = CTK.Table()
        row = [CTK.Box ({'class': 'icon_label'}, CTK.RawHTML (label))]

        if key in ['new_file','new_exts']:
            field = CTK.TextField({'name': key, 'class': 'noauto'})
            row.append(CTK.Box ({'class': 'icon_field'}, field))
            hidden_name = '%s_icon' %(key)
            prefix = ['icons!suffix', 'icons!file'][key == 'new_file']
        else:
            hidden_name = 'icons!%s' %(key)
            prefix = None

        table  += row
        submit  = CTK.Submitter (URL_APPLY)
        submit += table
        submit += IconChooser(hidden_name, prefix, **kwargs)
        self += submit


class AdditionDialogButton (CTK.Box):
    def __init__ (self, key, title, **kwargs):
        CTK.Box.__init__ (self, {'class': '%s-button' %(kwargs.get('klass', 'icon'))})
        submit_label = kwargs.get('submit_label', _('Add'))
        button_label = kwargs.get('button_label', submit_label)

        # Dialog
        dialog = CTK.Dialog ({'title': title, 'width': 375})
        dialog.AddButton (_('Cancel'), "close")
        dialog.AddButton (submit_label, dialog.JS_to_trigger('submit'))
        dialog += AddIcon(key, **kwargs)

        # Button
        button = CTK.Button (button_label)
        button.bind ('click', dialog.JS_to_show())
        dialog.bind ('submit_success', dialog.JS_to_close())
        dialog.bind ('submit_success', self.JS_to_trigger('submit_success'));

        self += button
        self += dialog


class Icons_Widget (CTK.Box):
    def __init__ (self, **kwargs):
        CTK.Box.__init__ (self, {'class':'icons'}, **kwargs)

        self += SpecialWidget_Instancer()
        self += FilesWidget_Instancer()
        self += ExtensionsWidget_Instancer()

CTK.publish ('^%s'%(URL_APPLY), commit, validation=VALIDATIONS, method="POST")
