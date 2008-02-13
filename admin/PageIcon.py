import os.path

from Page import *
from Form import *
from Table import *
from Entry import *
from configured import *

class PageIcon (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'icon', cfg)
        FormHelper.__init__ (self, 'icon', cfg)

    def _op_render (self):
        content = self._render_icon_list()

        self.AddMacroContent ('title', 'Icon configuration')
        self.AddMacroContent ('content', content)

        return Page.Render(self)

    def _op_handler (self, uri, post):
        if uri.startswith('/update'):
            return self._op_apply_changes (post)
        elif uri.startswith('/add_file'):
            return self._op_add_file (post)
        elif uri.startswith('/add_suffix'):
            return self._op_add_suffix (post)
        raise 'Unknown method'

    def _op_add_file (self, post):
        match = post['file_new_match'][0]
        file  = post['file_new_file'][0]

        # Add it to the config
        self._cfg["icons!file!%s" % (match)] = file
        # Return the URL
        return "/%s" % (self._id)

    def _op_add_suffix (self, post):
        exts = post['suffix_new_exts'][0]
        file = post['suffix_new_file'][0]

        # Add it to the config
        self._cfg["icons!suffix!%s" % (file)] = exts
        # Return the URL
        return "/%s" % (self._id)

    def _get_options_icons (self, cfg_key):
        # Build icons list
        file_options = [('', 'None')]
        for file in os.listdir (CHEROKEE_ICONSDIR):
            f = file.lower()
            if (f.endswith(".jpg") or
                f.endswith(".png") or
                f.endswith(".gif") or
                f.endswith(".svg")):
                file_options.append((file, file))

        # Build the options
        try:
            value   = self._cfg[cfg_key].value
            options = EntryOptions (cfg_key, file_options, 
                                    onChange='return update_icon(\'%s\');'%(cfg_key), 
                                    selected=value)
        except:
            value   = ''
            options = EntryOptions (cfg_key, file_options,
                                    onChange='return update_icon(\'%s\');'%(cfg_key))

        # Get the image
        image = self._get_img_from_icon (value, cfg_key)
        return options, image

    def _get_img_from_icon (self, icon_name, cfg_key):
        if not icon_name:
            return '<div id="image_%s"></div>'%(cfg_key)

        local_file  = os.path.join (CHEROKEE_ICONSDIR, icon_name)
        if not os.path.exists (local_file):
            return '<div id="image_%s"></div>'%(cfg_key)

        remote_file = os.path.join ('/icons_local', icon_name)
        return '<div id="image_%s"><img src="%s" /></div>' % (cfg_key, remote_file)

    def _render_icon_list (self):        
        # Include its JavaScript file
        #
        txt = '<script src="/static/js/icons.js" type="text/javascript"></script>'
        
        # Special icons
        #
        txt += '<h2>Special Icons</h2>'        

        table = Table(3)
        icons = self._cfg['icons']

        op_def, im_def = self._get_options_icons('icons!default')
        op_dir, im_dir = self._get_options_icons('icons!directory')
        op_par, im_par = self._get_options_icons('icons!parent_directory')

        table += (im_def, 'Default',      op_def)
        table += (im_dir, 'Directory',    op_dir)
        table += (im_par, 'Go to Parent', op_par)

        txt += str(table)
        
        # Files
        #
        table = Table(4, 1)
        table += ('', 'Match', 'File')
        icons = self._cfg['icons!file']

        txt += '<h2>Files</h2>'        
        if icons:
            for entry in icons:
                cfg_key = 'icons!file!%s' % (entry)
                op, im = self._get_options_icons (cfg_key)
                button = self.InstanceButton ("Del", "/%s/update"%(self._id))
                table += (im, entry, op, button)
            txt += str(table)

        # New file
        fo1 = Form ("/%s/add_file" % (self._id), add_submit=False)
        op1, im1 = self._get_options_icons ('file_new_file')
        en1 = self.InstanceEntry('file_new_match', 'text')
        ta1 = Table (4,1)
        ta1 += ('', 'File', 'Icon', '')
        ta1 += (im1, en1, op1, SUBMIT_ADD)
        txt += "<h3>Add file</h3>"
        txt += fo1.Render(str(ta1))

        # Suffixes
        #
        table = Table(4, 1)
        table += ('', 'File', 'Extensions', '')
        icons = self._cfg['icons!suffix']

        txt += '<h2>Suffixes</h2>'
        if icons:
            for icon in icons:
                cfg_key  = 'icons!suffix!%s' % (icon)
                im = self._get_img_from_icon (icon, cfg_key)

                entry    = self.InstanceEntry (cfg_key, 'text')
                button   = self.InstanceButton ('Del', '/%s/update'%(self._id))
                table += (im, icon, entry, button)
            txt += str(table)

        # New suffix
        fo1 = Form ("/%s/add_suffix" % (self._id), add_submit=False)
        op1, im1 = self._get_options_icons ('suffix_new_file')
        en2 = self.InstanceEntry('suffix_new_exts', 'text')
        ta1 = Table (4,1)
        ta1 += ('', 'Icon', 'Extensions', '')
        ta1 += (im1, op1, en2, SUBMIT_ADD)
        txt += "<h3>Add suffix</h3>"
        txt += fo1.Render(str(ta1))

        return txt

    def _op_apply_changes (self, post):
        self.ApplyChanges ([], post)
        return "/%s" % (self._id)
