import copy

from Form import *
from Table import *
from Page import *
from Module import *
from consts import *
import validations

class Rule (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'rule', cfg)
        Module.__init__ (self, 'rule', cfg, prefix, submit_url)

    def get_checks (self):
        matcher = self._cfg.get_val(self._prefix)
        if matcher in ['not', 'and', 'or']:
            return []
        if not matcher:
            return []

        rule_module = module_obj_factory (matcher, self._cfg, self._prefix, self.submit_url)
        if 'checks' in dir(rule_module):
            return rule_module.checks
        return []

    def get_title (self):
        txt = ''

        matcher = self._cfg.get_val(self._prefix)
        if not matcher:
            return "Unknown"

        if matcher == 'not':
            r = Rule(self._cfg, '%s!right'%(self._prefix), self.submit_url)
            return "Not (%s)"%(r.get_title())
        elif matcher in ['or', 'and']:
            op = ['AND', 'OR'][matcher == 'or']
            r1 = Rule(self._cfg, '%s!left'%(self._prefix), self.submit_url)
            r2 = Rule(self._cfg, '%s!right'%(self._prefix), self.submit_url)
            return "(%s) %s (%s)"%(r1.get_title(), op, r2.get_title())

        rule_module = module_obj_factory (matcher, self._cfg, self._prefix, self.submit_url)
        txt += "%s: %s" % (rule_module.get_type_name(), rule_module.get_name())
        return txt

    def get_name (self):
        matcher = self._cfg.get_val(self._prefix)
        if matcher in ['not', 'and', 'or']:
            return self.get_title()
        rule_module = module_obj_factory (matcher, self._cfg, self._prefix, self.submit_url)
        return rule_module.get_name()

    def get_type_name (self):
        matcher = self._cfg.get_val(self._prefix)
        if matcher in ['not', 'and', 'or']:
            return "Complex"
        rule_module = module_obj_factory (matcher, self._cfg, self._prefix, self.submit_url)
        return rule_module.get_type_name()

    def _op_render (self):
        txt = ""
        pre = self._prefix

        matcher = self._cfg.get_val(pre)
        if matcher == "not":
            rule = Rule (self._cfg, "%s!right"%(self._prefix), self.submit_url)
            rule_txt = rule._op_render()
            txt = """
            <table border="1" width="100%%">
             <tr><td>NOT*</td><td></td></tr>
             <tr><td></td><td>%(rule_txt)s</td></tr>
            </table>
            """ % (locals())
            return txt

        elif matcher in ["or", "and"]:
            op = ["AND", "OR"][matcher == "or"]
            
            rule1 = Rule (self._cfg, "%s!right"%(self._prefix), self.submit_url)
            rule1_txt = rule1._op_render()
            rule2 = Rule (self._cfg, "%s!left"%(self._prefix), self.submit_url)
            rule2_txt = rule2._op_render()

            txt = """
            <table border="1" width="100%%">
             <tr><td></td><td>%(rule1_txt)s</td></tr>
             <tr><td>%(op)s</td><td></td></tr>
             <tr><td></td><td>%(rule2_txt)s</td></tr>
            </table>
            """ % (locals())
            return txt

        if self._cfg.get_val(pre) == 'default':
            return self.Dialog (DEFAULT_RULE_WARNING, 'important-information')

        # Rule
        table = TableProps()
        e = self.AddPropOptions_Reload (table, "Rule Type", pre, modules_available(RULES), "")
        rule = self.Indent(str(table) + e)

        # Not
        _not = '<input type="button" value="Not" onClick="return rule_do_not(\'%s\');" />' % (pre)
        _and = '<input type="button" value="And" onClick="return rule_do_and(\'%s\');" />' % (pre)
        _or  = '<input type="button" value="Or" onClick="return rule_do_or(\'%s\');" />' % (pre)
        _del = '<input type="button" value="Remove" onClick="" />'

        txt += """
        <table border="1" width="100%%">
          <tr><td>%(_not)s</td><td></td><td></td></tr>
          <tr><td></td><td>%(rule)s</td><td>%(_del)s</td></tr>
          <tr><td></td><td align="right">%(_and)s , %(_or)s</td><td></td></tr>
        </table>""" % (locals())

        return txt

class RuleRender (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'rule_render', cfg)
        Module.__init__ (self, 'rule_render', cfg, prefix, submit_url)

    def _op_render (self):
        txt = ""
        return txt


class RuleOp (PageMenu, FormHelper):
    def __init__ (self, cfg):
        FormHelper.__init__ (self, 'rule_op', cfg)
        PageMenu.__init__ (self, 'rule_op', cfg, [])

    def __move (self, old, new):
        val = self._cfg.get_val(old)
        tmp = copy.deepcopy (self._cfg[old])
        del (self._cfg[old])

        self._cfg[new] = val
        self._cfg.set_sub_node (new, tmp)

    def _op_handler (self, uri, post):
        prefix = post['prefix'][0]

        if uri == "/not":
            self.__move (prefix, "%s!right"%(prefix))
            self._cfg[prefix] = "not"
        elif uri == '/and':
            self.__move (prefix, "%s!right"%(prefix))
            self._cfg[prefix] = "and"
        elif uri == '/or':
            self.__move (prefix, "%s!right"%(prefix))
            self._cfg[prefix] = "or"
        else:
            print "ERROR: Unknown uri '%s'"%(uri)

        return "ok"
