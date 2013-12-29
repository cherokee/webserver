# -*- coding: utf-8 -*-
#
# Cherokee-admin
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

import CTK
import Auth
import validations

from util import *
from consts import *

URL_APPLY = '/plugin/mysql/apply'
HELPS     = [('modules_validators_mysql', "MySQL")]

NOTE_HOST   = N_('MySQL server IP address.')
NOTE_PORT   = N_('Server port to connect to.')
NOTE_UNIX   = N_('Full path of Unix socket to communicate with the data base.')
NOTE_USER   = N_('User name for connecting to the database.')
NOTE_PASSWD = N_('Password for connecting to the database.')
NOTE_DB     = N_('Database name containing the user/password pair list.')
NOTE_SQL    = N_('SQL command to execute. ${user} is replaced with the user name, and ${passwd} is replaced with the user supplied password.')
NOTE_HASH   = N_('Choose an encryption type for the password. Only suitable for the "Basic" authentication mechanism.')

HASHES = [
    ('',       N_('None')),
    ('md5',    N_('MD5')),
    ('sha1',   N_('SHA1')),
    ('sha512', N_('SHA512'))
]


# Only allow to set a hash type when using 'basic'
BASIC_HASH_HACK = """
function mysql_hash_set_disabled() {
   var methods = $('#auth_method').val();
   if ((methods == "basic") || (methods == undefined)) {
       $('#mysql_hash').removeAttr("disabled");
   } else {
       $('#mysql_hash').attr("disabled", true);
   }
}

$('#auth_method').bind('change', function() {
   mysql_hash_set_disabled();
});

mysql_hash_set_disabled();
"""

class Plugin_mysql (Auth.PluginAuth):
    def __init__ (self, key, **kwargs):
        Auth.PluginAuth.__init__ (self, key, **kwargs)
        self.AddCommon (supported_methods=('basic','digest'))

        table = CTK.PropsTable()
        table.Add (_('Host'),          CTK.TextCfg("%s!host"%(self.key), True),        _(NOTE_HOST))
        table.Add (_('Port'),          CTK.TextCfg("%s!port"%(self.key), True),        _(NOTE_PORT))
        table.Add (_('Unix Socket'),   CTK.TextCfg("%s!unix_socket"%(self.key), True), _(NOTE_UNIX))
        table.Add (_('DB User'),       CTK.TextCfg("%s!user"%(self.key), False),       _(NOTE_USER))
        table.Add (_('DB Password'),   CTK.TextCfg("%s!passwd"%(self.key), True),      _(NOTE_PASSWD))
        table.Add (_('Database'),      CTK.TextCfg("%s!database"%(self.key), False),   _(NOTE_DB))
        table.Add (_('SQL Query'),     CTK.TextCfg("%s!query"%(self.key), False),      _(NOTE_SQL))
        table.Add (_('Password Hash'), CTK.ComboCfg("%s!hash"%(self.key), trans_options(HASHES), {'id': 'mysql_hash'}), _(NOTE_HASH))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML ("<h2>%s</h2>" % (_('MySQL Connection')))
        self += CTK.Indenter (submit)
        self += CTK.RawHTML (js=BASIC_HASH_HACK)

        # Publish
        VALS = [("%s!passwdfile"%(self.key), validations.is_local_file_exists)]
        CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, validation=VALS, method="POST")
