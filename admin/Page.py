from Form import *
from Theme import *
from Entry import *
from configured import *
from CherokeeManagement import *

# For gettext
N_ = lambda x: x

PAGE_BASIC_LAYOUT = """
    <div id="container">
	<div id="header">
	   <div id="logo"><a href="/"><img src="/static/images/cherokee-logo-bar.png" alt="logo"/></a></div>
	   <div id="version">%s {{version}}</div>
           {{help_block}}
	</div>
        <div id="bar">
        </div>
        <div id="workarea"><div id="workarea-inner">
          <div id="save_changes_msg"></div>
        {{content}}
        </div></div>
    </div>""" % (N_('Version:'))

PAGE_MENU_LAYOUT = """
    <div id="container">
	<div id="header">
	   <div id="logo"><a href="/"><img src="/static/images/cherokee-logo-bar.png" alt="logo"/></a></div>
	   <div id="version">%s {{version}}</div>
           {{help_block}}
	</div>
        <div id="bar">
            {{menu}}
        </div>
        <div id="workarea"><div id="workarea-inner">
          <div id="save_changes_msg"></div>
        {{content}}
        </div></div>
    </div>""" % (N_('Version:'))

PAGE_MENU_HELP_BLOCK = """
<div id="help">
{{help}}
</div>
"""

PAGE_MENU_MENU = """
<ul id="nav">
<li id="status"><a href="/">%s</a></li>
<li id="general"><a href="/general">%s</a></li>
<li id="vserver"><a href="/vserver">%s</a></li>
<li id="source"><a href="/source">%s</a></li>
<li id="icon"><a href="/icons">%s</a></li>
<li id="mime"><a href="/mime">%s</a></li>
<li id="advanced"><a href="/advanced">%s</a></li>
</ul>
<br />

<div id="changes_div">
<h2>%s</h2>
<div style="padding-top: 2px;">
 <p>{{menu_save_desc}}</p>
</div>

 {{menu_save_dropdown}}

 <div style="float: center; padding-top: 4px;">
  <a class="button" href="javascript:save_config();"><span>%s</span></a>
 </div>
</div>

 {{menu_available_languages}}

<script type="text/javascript">
  $(document).ready(protectChanges);
</script>
""" % (N_('Status'), N_('General'), N_('Virtual Servers'), N_('Information Sources'),
       N_('Icons'), N_('Mime Types'), N_('Advanced'), N_('Save Changes'), N_('Save'))

MENU_SAVE_IS_ALIVE = """
  <div style="padding-top: 2px;">
    <select name="restart" id="restart">
      <option value="graceful">%s</option>
      <option value="hard">%s</option>
      <option value="no">%s</option>
    </select>
  </div>
""" % (N_('Graceful restart'), N_('Hard restart'), N_('Do not restart'))

MENU_LANGUAGES = """
<div id="change_language">
  <h2>%s</h2>
  <div style="margin-top: 6px">
  <form name="flanguages" id="flanguages" method="post" action="/change_language">
    {{languages_select}}

    <div style="float: center; padding-top: 8px;">
        <a class="button" href="javascript:f = get_by_id('flanguages'); f.submit();"><span>%s</span></a>
    </div>
  </form>
  </div>
</div>
""" % (N_('Available languages'), N_('Change'))

DIALOG_W=725
DIALOG_H=500

class Page (WebComponent):
    def __init__ (self, id, cfg=None, helps=[]):
        WebComponent.__init__ (self, id, cfg)

        self.macros  = {}
        self.helps   = helps[:]
        self.theme   = Theme('default')

        self.AddMacroContent ('current', id)
        self.AddMacroContent ('version', VERSION)

    def Render (self):
        self._add_help_macro()
        return self.theme.BuildTemplate (self.macros, self._id)

    def AddMacroContent (self, key, value):
        self.macros[key] = value

    def Read (self, name):
        return self.theme.ReadFile(name)

    # Help
    #
    def AddHelp (self, (help_file, comment)):
        for h,c in self.helps:
            if h == help_file:
                return
        self.helps.append ((help_file, comment))

    def AddHelps (self, helps):
        for h in helps:
            self.AddHelp(h)

    def _render_help (self):
        if not self.helps:
            return ''
        txt = '<ul>'
        txt += '<li class="htop"><span>%s</span></li>' % (_('Help'))
        for hfile,comment in self.helps:
            if hfile.startswith("http://"):
                txt += '<li><a href="%s" target="_help">%s</a></li>' % (hfile, comment)
            else:
                params=(hfile, DIALOG_H, DIALOG_W, comment)
                txt += '<li class="hitem"><a href="/help/%s.html?KeepThis=true&TB_iframe=true&height=%s&width=%s" class="thickbox">%s</a></li>' % params
        txt += '</ul>'
        return txt

    def _add_help_macro (self):
        help_txt = self._render_help()
        if help_txt:
            self.AddMacroContent ('help_block', PAGE_MENU_HELP_BLOCK)
        else:
            self.AddMacroContent ('help_block', '')
        self.AddMacroContent ('help', help_txt)


class PageMenu (Page):
    def __init__ (self, id, cfg, helps=[]):
        Page.__init__ (self, id, cfg, helps)

        manager = cherokee_management_get (cfg)
        if manager.is_alive():
            self.AddMacroContent ('menu_save_dropdown', MENU_SAVE_IS_ALIVE)
            self.AddMacroContent ('menu_save_desc', _('Commit to disk and apply changes to the running server'))
        else:
            self.AddMacroContent ('menu_save_dropdown', '')
            self.AddMacroContent ('menu_save_desc', _('Commit all the changes permanently'))

        if len(AVAILABLE_LANGUAGES) > 1:
            self.AddMacroContent ('menu_available_languages', MENU_LANGUAGES)
            selbox = EntryOptions ('language', AVAILABLE_LANGUAGES)
            self.AddMacroContent ('languages_select', str(selbox))
        else:
            self.AddMacroContent ('menu_available_languages', '')

        self.AddMacroContent ('body', PAGE_MENU_LAYOUT)
        self.AddMacroContent ('menu', PAGE_MENU_MENU)
