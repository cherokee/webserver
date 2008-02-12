from Page import *
from Form import *
from Table import *
from Entry import *

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

    def _render_icon_list (self):        
        # Special icons
        #
        table = Table(2, 1)
        table += ('', 'Icon file')
        icons = self._cfg['icons']

        self.AddTableEntry (table, 'Default', 'icons!default')
        self.AddTableEntry (table, 'Directory', 'icons!directory')
        self.AddTableEntry (table, 'Parent Dir.', 'icons!parent_directory')

        form = Form ("/%s/update" % (self._id))
        txt = '<h2>Special Icons</h2>'        
        txt += form.Render(str(table))
        
        # Files
        #
        table = Table(3, 1)
        table += ('Match', 'File', '')
        icons = self._cfg['icons!file']

        txt += '<h2>Files</h2>'        
        if icons:
            for entry in icons:
                self.AddTableEntryRemove (table, entry, 'icons!file!%s' % (entry))
            txt += str(table)

        fo1 = Form ("/%s/add_file" % (self._id), add_submit=False)
        en1 = self.InstanceEntry('file_new_match', 'text')
        en2 = self.InstanceEntry('file_new_file', 'text')
        ta1 = Table (3,1)
        ta1 += ('File', 'Icon', '')
        ta1 += (en1, en2, SUBMIT_ADD)
        txt += "<h3>Add file</h3>"
        txt += fo1.Render(str(ta1))

        # Suffixes
        #
        table = Table(3, 1)
        table += ('File', 'Extensions', '')
        icons = self._cfg['icons!suffix']

        txt += '<h2>Suffixes</h2>'
        if icons:
            for entry in icons:
                self.AddTableEntryRemove (table, entry, 'icons!suffix!%s' % (entry))
            txt += str(table)

        fo1 = Form ("/%s/add_suffix" % (self._id), add_submit=False)
        en1 = self.InstanceEntry('suffix_new_file', 'text')
        en2 = self.InstanceEntry('suffix_new_exts', 'text')
        ta1 = Table (3,1)
        ta1 += ('Icon', 'Extensions', '')
        ta1 += (en1, en2, SUBMIT_ADD)
        txt += "<h3>Add suffix</h3>"
        txt += fo1.Render(str(ta1))

        return txt

    def _op_apply_changes (self, post):
        self.ApplyChanges ([], post)
        return "/%s" % (self._id)
