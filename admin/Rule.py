import copy

from Form import *
from Table import *
from Page import *
from Module import *
from consts import *
import validations

class Rule (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url, depth):
        FormHelper.__init__ (self, 'rule', cfg)
        Module.__init__ (self, 'rule', cfg, prefix, submit_url)
        self.depth = depth

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
            r = Rule(self._cfg, '%s!right'%(self._prefix), self.submit_url, self.depth+1)
            return "Not (%s)"%(r.get_title())
        elif matcher in ['or', 'and']:
            op = ['AND', 'OR'][matcher == 'or']
            r1 = Rule(self._cfg, '%s!left'%(self._prefix), self.submit_url, self.depth+1)
            r2 = Rule(self._cfg, '%s!right'%(self._prefix), self.submit_url, self.depth+1)
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
            rule = Rule (self._cfg, "%s!right"%(self._prefix), self.submit_url, self.depth+1)
            rule_txt = rule._op_render()
            txt = """
            <div class="rule_not">
              <div class="rule_not_title">NOT</div>
              %(rule_txt)s
            </div>
            """ % (locals())
            return txt

        elif matcher in ["or", "and"]:
            op = ["AND", "OR"][matcher == "or"]
            
            depth = self.depth + 1
            rule1 = Rule (self._cfg, "%s!right"%(self._prefix), self.submit_url, depth)
            rule1_txt = rule1._op_render()
            rule2 = Rule (self._cfg, "%s!left"%(self._prefix), self.submit_url, depth)
            rule2_txt = rule2._op_render()

            txt = """
            <div class="rule_group rule_group_%(depth)s">
              %(rule1_txt)s
              <div class="rule_operation">%(op)s</div>
              %(rule2_txt)s
            </div>
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
        <div class="rule_box">
          <div class="rule_content">%(rule)s</div>
          <div class="rule_toolbar">%(_not)s%(_and)s%(_or)s%(_del)s</div>
        </div>""" % (locals())

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
