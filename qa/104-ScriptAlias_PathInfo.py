import os.path
from base import *

DIR       = "alias_and_pathinfo"
PATH_INFO = "/this_is_the/path_info"

CONF = """
vserver!1!rule!1040!match = directory
vserver!1!rule!1040!match!directory = /alias_and_pathinfo
vserver!1!rule!1040!handler = cgi
vserver!1!rule!1040!handler!script_alias = %s
"""
CGI_BASE = """#!/bin/sh
echo "Content-Type: text/plain"
echo
echo "PATH_INFO = -${PATH_INFO}-"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "CGI: ScriptAlias with PathInfo"

        self.request          = "GET /%s%s HTTP/1.0\r\n" % (DIR, PATH_INFO)
        self.expected_error   = 200
        self.expected_content = "PATH_INFO = -%s-" % (PATH_INFO)

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, "exec.cgi", 0755, CGI_BASE)
        self.conf = CONF % (f)
