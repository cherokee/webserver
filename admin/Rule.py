import copy

from Form import *
from Table import *
from Page import *
from Module import *
from consts import *
import validations

DEFAULT_RULE_WARNING = 'The default match ought not to be changed.'

class Rule (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url, depth=0):
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
        name = rule_module.get_name()

        if not name:
            name = "Undefined.."

        return name

    def get_type_name (self):
        matcher = self._cfg.get_val(self._prefix)
        if matcher in ['not', 'and', 'or']:
            return "Complex"
        rule_module = module_obj_factory (matcher, self._cfg, self._prefix, self.submit_url)
        return rule_module.get_type_name()

    def _get_ops (self, pre):
        _not = '<input type="button" value="Not" onClick="return rule_do_not(\'%s\');" />' % (pre)
        _and = '<input type="button" value="And" onClick="return rule_do_and(\'%s\');" />' % (pre)
        _or  = '<input type="button" value="Or" onClick="return rule_do_or(\'%s\');" />' % (pre)
        _del = '<input type="button" value="Remove" onClick="return rule_delete(\'%s\');" />' %(pre)
        return (_not,_and,_or,_del)

    def _op_render (self):
        txt = ""
        pre = self._prefix

        matcher = self._cfg.get_val(pre)
        if matcher == "not":
            rule = Rule (self._cfg, "%s!right"%(self._prefix), self.submit_url, self.depth+1)
            rule_txt = rule._op_render()

            _not, _and, _or, _del = self._get_ops(pre)

            txt = """
            <div class="rule_group rule_not">
              <div class="rule_not_title">NOT</div>
              %(rule_txt)s
              <div class="rule_toolbar">%(_and)s %(_or)s %(_del)s</div>
            </div>
            """ % (locals())
            return txt

        elif matcher in ["or", "and"]:
            op = ["AND", "OR"][matcher == "or"]
            
            depth = self.depth + 1
            rule1 = Rule (self._cfg, "%s!left"%(self._prefix), self.submit_url, depth)
            rule1_txt = rule1._op_render()
            rule2 = Rule (self._cfg, "%s!right"%(self._prefix), self.submit_url, depth)
            rule2_txt = rule2._op_render()

            _not, _and, _or, _del = self._get_ops(pre)

            prev = '!'.join(pre.split('!')[:-1])
            prev_rule = self._cfg.get_val(prev)
            if prev_rule == "not":
                _not = ''

            txt = """
            <div class="rule_group rule_group_%(depth)s">
              %(rule1_txt)s
              <div class="rule_operation">%(op)s</div>
              %(rule2_txt)s
              <div class="rule_toolbar">%(_not)s %(_del)s</div>
            </div>
            """ % (locals())
            return txt

        # Special Case: Default rule
        if self._cfg.get_val(pre) == 'default':
            return self.Dialog (DEFAULT_RULE_WARNING, 'important-information')

        # The rule hasn't been set
        if not self._cfg.get_val(pre):
            self._cfg[pre] = 'directory'

        # Rule
        table = TableProps()
        e = self.AddPropOptions_Reload (table, "Rule Type", pre, modules_available(RULES), "")
        rule = self.Indent(str(table) + e)

        # Operations
        _not, _and, _or, _del = self._get_ops(pre)

        # Allow to remove rules only if they are inside an AND or OR.
        prev = '!'.join(pre.split('!')[:-1])
        prev_rule = self._cfg.get_val(prev)
        if not prev_rule in ['and', 'or']:
            _del = ''
            if "!match" in prev:
                _not = ''

        txt += """
        <div class="rule_box rule_group">
          <div class="rule_content">%(rule)s</div>
          <div class="rule_toolbar">%(_not)s %(_and)s %(_or)s %(_del)s</div>
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
            if self._cfg.get_val(prefix) == "not":
                # not(not(rule)) == rule
                self.__move ("%s!right"%(prefix), prefix)
            else:
                self.__move (prefix, "%s!right"%(prefix))
                self._cfg[prefix] = "not"

        elif uri == '/and':
            self.__move (prefix, "%s!left"%(prefix))
            self._cfg[prefix] = "and"
        elif uri == '/or':
            self.__move (prefix, "%s!left"%(prefix))
            self._cfg[prefix] = "or"

        elif uri == "/del":
            rule = self._cfg.get_val(prefix)
            if rule == "not":
                self.__move ("%s!right"%(prefix), prefix)
            elif rule in ['and', 'or']:
                self.__move ("%s!left"%(prefix), prefix)
            else:
                del(self._cfg[prefix])
            
        else:
            print "ERROR: Unknown uri '%s'"%(uri)

        return "ok"
