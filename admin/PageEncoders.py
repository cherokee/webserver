import validations

from Page import *
from Form import *
from Table import *
from consts import *

DATA_VALIDATION = [
]

COMMENT = """
<p>In this section you can configure the server encoders, which
basically means that you can define where you want it to compress the
information being sent with GZip.</p>
"""

DATA_VALIDATION = [
    ("server!encoder!.*?!allow", validations.is_safe_id_list),
    ("server!encoder!.*?!deny",  validations.is_safe_id_list)
]

NOTE_MATCH_TYPE  = 'Define how the following rules are going to be evaluated.'
NOTE_MATCH_ALLOW = 'List of extensions that will be encoded.'
NOTE_MATCH_DENY  = 'List of extensions that will not be encoded.'

class MatchingList (FormHelper):
    OPTIONS = [
        ('default_deny',  'Deny by default'),
        ('default_allow', 'Allow by default'),
        ('deny_allow',    'Deny, Allow'),
        ('allow_deny',    'Allow, Deny')
    ]

    def __init__ (self, cfg, pre, errors):
        FormHelper.__init__ (self, 'matching_list', cfg)
        self._prefix = pre
        self.errors  = errors
        
    def _op_render (self):
        txt   = ''
        table = TableProps()
        self.AddPropOptions (table, 'Type',  '%s!type' %(self._prefix), self.OPTIONS, NOTE_MATCH_TYPE)
        self.AddPropEntry   (table, 'Allow', '%s!allow'%(self._prefix), NOTE_MATCH_ALLOW)
        self.AddPropEntry   (table, 'Deny',  '%s!deny' %(self._prefix), NOTE_MATCH_DENY)
        txt += str(table)

        return txt


class PageEncoders (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'encoder', cfg)
        FormHelper.__init__ (self, 'encoder', cfg)

    def _op_render (self):
        content = self._render_encoder_list()

        self.AddMacroContent ('title', 'Encoders configuration')
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _op_handler (self, uri, post):
        if post.get_val('is_submit') or \
           uri.startswith('/update'):
            self._op_apply_changes (post)
        elif uri.startswith('/add_encoder'):
            self._op_apply_add_encoder (post)
        else:
            raise 'Unknown method'
        return self._op_render()        

    def _render_encoder_list (self):
        txt     = ''
        cfg_key = 'server!encoder'
        cfg     = self._cfg[cfg_key]

        txt += "<h1>Information encoding</h1>"
        txt += self.Dialog(COMMENT)
        
        # Current encoders
        if cfg and cfg.has_child():
            txt += "<h2>Encoders</h2>"
            
            encs_txt = []
            for encoder in cfg:
                txt2    = ''
                cfg_key = '%s!%s'%(cfg_key, encoder)

                mlist = MatchingList (self._cfg, cfg_key, self.errors)
                txt2 += "<h3>%s</h3>" % (encoder)
                txt2 += mlist._op_render()
                js = "post_del_key('/%s/update', '%s');" % (self._id, cfg_key)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                txt2 += link_del
                encs_txt.append(txt2)

            txt += self.Indent("<hr />".join(encs_txt))

        # Add new encoder
        if not cfg:
            encoders_left = ENCODERS
        else:
            encoders_left = []
            for i in range(len(ENCODERS)):
                encoder, desc = ENCODERS[i]
                if not encoder in cfg:
                    encoders_left.append (ENCODERS[i])

        if encoders_left:
            txt += "<h2>Add encoder</h2>"

            table = Table(2)
            ops = EntryOptions ("new_encoder", encoders_left)

            js = "post_add_entry_key('/%s/add_encoder', 'new_encoder', 'new_encoder');" % (self._id)
            bu1 = self.InstanceButton ("Add", onClick=js)

            table += (ops, bu1)
            txt += self.Indent(str(table))

        form = Form ("/%s" % (self._id))
        return form.Render(txt,DEFAULT_SUBMIT_VALUE)
	
    def _op_apply_changes (self, post):
        self.ApplyChanges ([], post, DATA_VALIDATION)
        return "/%s" % (self._id)
    
    def _op_apply_add_encoder (self, post):
        encoder = post.get_val('new_encoder')
        if not encoder:
            return "/%s" % (self._id)

        self._cfg['server!encoder!%s!type'%(encoder)] = MatchingList.OPTIONS[0][0]
        return "/%s" % (self._id)
