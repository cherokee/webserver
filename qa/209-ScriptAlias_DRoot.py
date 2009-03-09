import os.path
from base import *

TOP_DIR     = "209_top_dir"
DIR         = "209_scripalias_documentroot"
PATH_INFO   = "/this_is_the/path_info"

CONF = """
vserver!1!rule!2090!match = directory
vserver!1!rule!2090!match!directory = /%s
vserver!1!rule!2090!handler = cgi
vserver!1!rule!2090!handler!script_alias = %s
vserver!1!rule!2090!document_root = %s
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
        d = self.Mkdir (www, TOP_DIR)
        f = self.WriteFile (d, "exec.cgi", 0755, CGI_BASE)
        self.conf = CONF % (DIR, f, d)
