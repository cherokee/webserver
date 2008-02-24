from Form import *
from Table import *
from Module import *

NOTE_ON_PCRE = """
<p>Regular expressions must be written using the <a href="http://www.pcre.org/">
PCRE</a> syntax. Here you can find information on the 
<a target="_blank" href="http://perldoc.perl.org/perlre.html"> regular expression 
format</a>.</p>
"""



class ModuleRedir (Module, FormHelper):
    PROPERTIES = [
        "rewrite"
    ]

    def __init__ (self, cfg, prefix, submit_url):
        Module.__init__ (self, 'redir', cfg, prefix, submit_url)
        FormHelper.__init__ (self, 'redir', cfg)
        
    def _op_render (self):
        cfg_key = "%s!rewrite" % (self._prefix)
        cfg     = self._cfg[cfg_key]
        txt     = ''

        # Current rules
        if cfg and cfg.has_child():
            table = Table (4,1)
            table += ('Show', 'Regular Expression', 'Substitution', '')

            for rule in cfg:
                cfg_key_rule = "%s!%s" % (cfg_key, rule)

                show      = self.InstanceCheckbox ('%s!show'%(cfg_key_rule))
                regex     = self._cfg.get_val('%s!regex'    %(cfg_key_rule))
                substring = self._cfg.get_val('%s!substring'%(cfg_key_rule))
                js = "post_del_key('/ajax/update', '%s');" % (cfg_key_rule)
                button    = self.InstanceButton ('Del', onClick=js)
                table += (show, regex, substring, button)

            txt += "<h3>Rule list</h3>"
            txt += self.Indent(table)

        # Add new rule
        en_reg  = self.InstanceEntry('rewrite_new_regex',     'text')
        en_sub  = self.InstanceEntry('rewrite_new_substring', 'text')
        en_show = self.InstanceCheckbox ('rewrite_new_show')

        table = Table(3,1)
        table += ('Show', 'Regular Expression', 'Substitution')
        table += (en_show, en_reg, en_sub)

        txt += "<h3>Add new rule</h3>"
        txt += self.Dialog(NOTE_ON_PCRE)
        txt += self.Indent(table)
        return txt

    def __find_name (self):
        i = 1
        while True:
            key = "%s!rewrite!%d" % (self._prefix, i)
            tmp = self._cfg[key]
            if not tmp: 
                return str(i)
            i += 1

    def _op_apply_changes (self, uri, post):
        regex  = None
        substr = None
        show   = "0"

        if 'rewrite_new_regex' in post and \
           len(post['rewrite_new_regex'][0]) > 0:
           regex = post['rewrite_new_regex'][0]
           del (post['rewrite_new_regex'])

        if 'rewrite_new_substring' in post and \
           len(post['rewrite_new_substring'][0]) > 0:
           substr = post['rewrite_new_substring'][0]
           del (post['rewrite_new_substring'])
        
        if regex or substr:
            if 'rewrite_new_show' in post and \
               len(post['rewrite_new_show']) > 0:
                s = post['rewrite_new_show'][0]
                if s in ['on', '1']:
                    show = "1"
                else:
                    show = "0"
                del(post['rewrite_new_show'])

            pre = "%s!rewrite!%s" % (self._prefix, self.__find_name())

            self._cfg['%s!show'%(pre)] = show
            if regex:
                self._cfg['%s!regex'%(pre)] = regex
            if substr:
                self._cfg['%s!substring'%(pre)] = substr

        self.ApplyChangesPrefix (self._prefix, [], post)
