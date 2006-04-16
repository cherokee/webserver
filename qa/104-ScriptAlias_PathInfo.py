import os.path
from base import *

PATH_INFO   = "/this_is_the/path_info"

CONF = """
vserver!default!directory!/alias_and_pathinfo!handler = cgi
vserver!default!directory!/alias_and_pathinfo!handler!script_alias = %s
vserver!default!directory!/alias_and_pathinfo!priority = 1040
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "CGI: ScriptAlias with PathInfo"

        self.request          = "GET /alias_and_pathinfo%s HTTP/1.0\r\n" % (PATH_INFO)
        self.expected_error   = 200
        self.expected_content = "PATH_INFO = -%s-" % (PATH_INFO)

    def Prepare (self, www):
        d = self.Mkdir (www, "alias_and_pathinfo")
        f = self.WriteFile (d, "exec.cgi", 0755,
                            """#!/bin/sh

                            echo "Content-Type: text/plain"
                            echo
                            echo "PATH_INFO = -${PATH_INFO}-"
                            """) 

        self.conf = CONF % (f)
