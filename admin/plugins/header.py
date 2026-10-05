# Cheroke Admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
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

from CTK.i18n import _, N_
import CTK

from Rule import RulePlugin
from util import *

URL_APPLY = '/plugin/header/apply'

NOTE_HEADER   = N_("Header against which the regular expression will be evaluated.")
NOTE_MATCH    = N_("Regular expression.")
NOTE_TYPES    = N_("It defines how this match has to be evaluated.")
NOTE_COMPLETE = N_("Match against the complete header block, rather than against a concrete header entry.")

HEADERS = [
    ('Accept-Encoding',   'Accept-Encoding'),
    ('Accept-Charset',    'Accept-Charset'),
    ('Accept-Language',   'Accept-Language'),
    ('Accept',            'Accept'),
    ('Authorization',     'Authorization'),
    ('Cache-Control',     'Cache-Control'),
    ('Connection',        'Connection'),
    ('Content-Encoding',  'Content-Encoding'),
    ('Content-Length',    'Content-Length'),
    ('Content-Type',      'Content-Type'),
    ('Cookie',            'Cookie'),
    ('DNT',               'DNT'),
    ('Expect',            'Expect'),
    ('Host',              'Host'),
    ('If-Modified-Since', 'If-Modified-Since'),
    ('If-None-Match',     'If-None-Match'),
    ('If-Range',          'If-Range'),
    ('Keep-Alive',        'Keep-Alive'),
    ('Location',          'Location'),
    ('Range',             'Range'),
    ('Referer',           'Referer'),
    ('Set-Cookie',        'Set-Cookie'),
    ('Transfer-Encoding', 'Transfer-Encoding'),
    ('Upgrade',           'Upgrade'),
    ('User-Agent',        'User-Agent'),
    ('X-Forwarded-For',   'X-Forwarded-For'),
    ('X-Forwarded-Host',  'X-Forwarded-Host'),
    ('X-Real-IP',         'X-Real-IP')
]

TYPES = [
    ('regex',    _('Matches a Regular Expression')),
    ('provided', _('Is Provided'))
]

def commit():
    # POST info
    key          = CTK.post.pop ('key', None)
    vsrv_num     = CTK.post.pop ('vsrv_num', None)
    new_hdr      = CTK.post.pop ('tmp!header', None)
    new_match    = CTK.post.pop ('tmp!match', None)
    new_type     = CTK.post.pop ('tmp!type', None)
    new_complete = CTK.post.pop ('tmp!complete', None)

    # New entry
    if new_hdr:
        next_rule, next_pre = cfg_vsrv_rule_get_next (f'vserver!{vsrv_num}')

        CTK.cfg[f'{next_pre}!match']          = 'header'
        CTK.cfg[f'{next_pre}!match!header']   = new_hdr
        CTK.cfg[f'{next_pre}!match!match']    = new_match
        CTK.cfg[f'{next_pre}!match!type']     = new_type
        CTK.cfg[f'{next_pre}!match!complete'] = new_complete

        return {'ret': 'ok', 'redirect': f'/vserver/{vsrv_num}/rule/{next_rule}'}

    # Modifications
    return CTK.cfg_apply_post()


class Plugin_header (RulePlugin):
    def __init__ (self, key, **kwargs):
        RulePlugin.__init__ (self, key)
        is_new = key.startswith('tmp')
        props  = ({},{'class': 'noauto'})[is_new]

        type_widget  = CTK.ComboCfg (f'{key}!type', TYPES, props)
        type_val     = CTK.cfg.get_val (f'{key}!type', 'regex')
        complete_val = int (CTK.cfg.get_val (f'{key}!complete', '0'))

        complete = CTK.CheckCfgText (f'{key}!complete', False, _('Full header'), props)

        table = CTK.PropsTable()
        table.Add (_("Complete Header"), complete, _(NOTE_COMPLETE))

        if is_new or not complete_val:
            table.Add (_('Header'), CTK.ComboCfg(f'{key}!header', HEADERS, props), _(NOTE_HEADER))

        if is_new or not complete_val:
            table.Add (_('Type'), type_widget, _(NOTE_TYPES))

        if is_new or type_val == 'regex' or complete_val:
            table.Add (_('Regular Expression'), CTK.TextCfg(f'{key}!match', False, props), _(NOTE_MATCH))

        if is_new:
            type_widget.bind ('change',
                  """if ($('#{type_widget}').val() == 'regex') {{
                           $('#{regex}').show();
                     }} else {{
                           $('#{regex}').hide();
                  }}""".format(type_widget=type_widget.id, regex=table[2].id))

            complete.bind ('change',
                  """if (! $('#{complete} :checkbox')[0].checked) {{
                           $('#{row1}').show();
                           $('#{row2}').show();
                     }} else {{
                           $('#{row1}').hide();
                           $('#{row2}').hide();
                  }}""".format(complete=complete.id, row1=table[1].id, row2=table[2].id))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('key', key)
        submit += CTK.Hidden ('vsrv_num', kwargs.pop('vsrv_num', ''))
        submit += table
        self += submit

        # Validation, and Public URLs
        CTK.publish (f"^{URL_APPLY}", commit, method="POST")

    def GetName (self):
        header   = CTK.cfg.get_val (f'{self.key}!header', '')
        match    = CTK.cfg.get_val (f'{self.key}!match', '')
        type_    = CTK.cfg.get_val (f'{self.key}!type', '')
        complete = int (CTK.cfg.get_val (f'{self.key}!complete', '0'))

        if complete:
            return f"{_('Header')} ~ {match}"

        if type_ == 'provided':
            return _("Header %(header)s is provided") %(locals())

        return f"{_('Header')} {header} ~ {match}"
