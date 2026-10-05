# -*- coding: utf-8 -*-

from qa_base import *

DIR      = "设为首"
PATHINFO = "/Ĥēĺļŏ/Ŵőřļď"

CONF = f"""
vserver!1!rule!2420!match = directory
vserver!1!rule!2420!match!directory = /{DIR}
vserver!1!rule!2420!handler = cgi
"""

CGI_BASE = """#!/bin/sh
echo "Content-Type: text/plain"
echo
echo "PathInfo is $PATH_INFO"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "UTF8 dir match + pathinfo"

        self.request          = f"GET /{DIR}/test{PATHINFO} HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 200
        self.expected_content = f"PathInfo is {PATHINFO}"

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test", 0o755, CGI_BASE)
