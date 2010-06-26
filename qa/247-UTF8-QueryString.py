# -*- coding: utf-8 -*-

from base import *
from urllib import quote

UTF8   = "¿Quién Está al Teléfono?"
DIR    = "querystring-utf8-1"
PARAMS = "param1=%s" %(quote(UTF8))

CONF = """
vserver!1!rule!2470!match = directory
vserver!1!rule!2470!match!directory = /%s
vserver!1!rule!2470!handler = cgi
""" %(DIR)

CGI_BASE = """#!/bin/sh
# %s
echo "Content-Type: text/plain"
echo
echo "QUERY_STRING = $QUERY_STRING"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "CGI: QUERY_STRING - UTF8 param"

        self.request          = "GET /%s/exec.cgi?%s HTTP/1.0\r\n" % (DIR, PARAMS)
        self.conf             = CONF
        self.expected_error   = 200
        self.expected_content = "QUERY_STRING = %s" % (PARAMS)

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "exec.cgi", 0755, CGI_BASE)
