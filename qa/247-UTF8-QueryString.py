# -*- coding: utf-8 -*-

from qa_base import *
from urllib.parse import quote

UTF8   = "¿Quién Está al Teléfono?"
DIR    = "querystring-utf8-1"
PARAMS = f"param1={quote(UTF8)}"

CONF = f"""
vserver!1!rule!2470!match = directory
vserver!1!rule!2470!match!directory = /{DIR}
vserver!1!rule!2470!handler = cgi
"""

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

        self.request          = f"GET /{DIR}/exec.cgi?{PARAMS} HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 200
        self.expected_content = f"QUERY_STRING = {PARAMS}"

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "exec.cgi", 0o755, CGI_BASE)
