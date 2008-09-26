import os.path

from Page import *
from Form import *
from Table import *
from Entry import *
from configured import *

HELPS = [('config_icons', "Icons")]

class PageIcon (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'icon', cfg, HELPS)
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

        raise Exception ('Unknown method')

    def _op_add_file (self, post):
        match = post.get_val('file_new_match')
        file  = post.get_val('file_new_file')

        # Add it to the config
        self._cfg["icons!file!%s" % (file)] = match
        # Return the URL
        return "/%s" % (self._id)

    def _op_add_suffix (self, post):
        exts = post['suffix_new_exts'][0]
        file = post['suffix_new_file'][0]

        # Add it to the config
        self._cfg["icons!suffix!%s" % (file)] = exts
        # Return the URL
        return "/%s" % (self._id)

    def _get_options_icons (self, cfg_key, file_filter=None, selected=None):
        file_options = []

        # No icon
        if file_filter and not file_filter(''):
            file_options.append (('', 'None'))

        # Build icons list
        for file in os.listdir (CHEROKEE_ICONSDIR):
            if file_filter:
                ignore = file_filter (file)
                if ignore: continue

            f = file.lower()
            if (f.endswith(".jpg") or
                f.endswith(".png") or
                f.endswith(".gif") or
                f.endswith(".svg")):
                file_options.append((file, file))

        # Check selected
        if not selected:
            selected = self._cfg.get_val(cfg_key)
            
        # Build the options
        if selected:
            options = EntryOptions (cfg_key, file_options, 
                                    onChange='return option_icons_update(\'%s\');'%(cfg_key), 
                                    selected=selected)
        else:
            options = EntryOptions (cfg_key, file_options,
                                    onChange='return option_icons_update(\'%s\');'%(cfg_key))

        # Get the image
        image = self._get_img_from_icon (selected, cfg_key)
        return options, image

    def _get_img_from_icon (self, icon_name, cfg_key):
        if not icon_name:
            return '<div id="image_%s"></div>'%(cfg_key)

        local_file  = os.path.join (CHEROKEE_ICONSDIR, icon_name)
        if not os.path.exists (local_file):
            comment = "<!-- Couldn't find %s -->" % (icon_name)
            return '<div id="image_%s">%s</div>' % (cfg_key, comment)

        remote_file = os.path.join ('/icons_local', icon_name)
        return '<div id="image_%s"><img src="%s" /></div>' % (cfg_key, remote_file)

    def _render_icon_list (self):        
        # Include its JavaScript file
        #
        txt  = '<script src="/static/js/icons.js" type="text/javascript"></script>'
        txt += "<h1>Icons configuration</h1>"

        tabs = []
        
        # Suffixes
        #
        icons = self._cfg['icons!suffix']

        tmp = ''
        if icons and icons.has_child():
            tmp += "<h3>Extension list</h3>"

            table = Table(4, 1)
            table += ('', 'File', 'Extensions', '')

            for icon in icons:
                cfg_key  = 'icons!suffix!%s' % (icon)
                im = self._get_img_from_icon (icon, cfg_key)

                entry  = self.InstanceEntry (cfg_key, 'text', size=45)
                js     = "post_del_key('/icons/update', '%s');" % (cfg_key)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                table += (im, icon, entry, link_del)

            tmp += self.Indent(table)

        # New suffix
        fo1 = Form ("/%s/add_suffix" % (self._id), add_submit=False, auto=False)
        op1, im1 = self._get_options_icons ('suffix_new_file', 
                                            self._filter_icons_in_suffixes)
        en2 = self.InstanceEntry('suffix_new_exts', 'text')
        ta1 = Table (4,1)
        ta1 += ('', 'Icon', 'Extensions', '')
        ta1 += (im1, op1, en2, SUBMIT_ADD)
        tmp += "<h3>Add suffix</h3>"
        tmp += fo1.Render(self.Indent(ta1))

        tabs += [('Extensions', tmp)]

        # Special icons
        #
        table = Table(3, 1)
        icons = self._cfg['icons']

        op_def, im_def = self._get_options_icons('icons!default')
        op_dir, im_dir = self._get_options_icons('icons!directory')
        op_par, im_par = self._get_options_icons('icons!parent_directory')

        table += ('', 'Icon', 'File')
        table += (im_def, 'Default',      op_def)
        table += (im_dir, 'Directory',    op_dir)
        table += (im_par, 'Go to Parent', op_par)

        tmp  = "<h3>Special Entries</h3>"
        tmp += self.Indent(table)

        tabs += [('Special Icons', tmp)]
        
        # Files
        #
        icons = self._cfg['icons!file']

        tmp = ''
        if icons and icons.has_child():
            tmp += "<h3>File list</h3>"

            table = Table(4, 1)
            table += ('', 'Match', 'File')

            for icon_name in icons:
                cfg_key = 'icons!file!%s' % (icon_name)
                match   = self._cfg.get_val(cfg_key)
                op, im  = self._get_options_icons (cfg_key, selected=icon_name)
                js = "post_del_key('/icons/update', '%s');" % (cfg_key)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                table += (im, match, op, link_del)

            tmp += self.Indent(table)

        # New file
        fo1 = Form ("/%s/add_file" % (self._id), add_submit=False, auto=False)
        op1, im1 = self._get_options_icons ('file_new_file')
        en1 = self.InstanceEntry('file_new_match', 'text')
        ta1 = Table (4,1)
        ta1 += ('', 'File', 'Icon', '')
        ta1 += (im1, en1, op1, SUBMIT_ADD)
        tmp += "<h3>Add file</h3>"
        tmp += fo1.Render(self.Indent(ta1))

        tabs += [('Files', tmp)]


        txt += self.InstanceTab (tabs)
        return txt

    def _op_apply_changes (self, post):
        self.ApplyChanges ([], post)
        return "/%s" % (self._id)

    def _filter_icons_in_suffixes (self, file):
        cfg = self._cfg['icons!suffix']
        if not cfg:
            return False
        if not file:
            return True
        return file in cfg.keys()
