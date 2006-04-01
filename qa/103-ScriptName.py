from base import *

PATHINFO    = "this_is_the/path_info"
VIRTUAL_DIR = "/scriptname_vir"
SCRIPT_NAME = VIRTUAL_DIR + "/exec.cgi"


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "CGI: ScriptName with PathInfo"

        self.request          = "GET %s/exec.cgi/%s HTTP/1.0\r\n" % (VIRTUAL_DIR, PATHINFO)
        self.expected_error   = 200
        self.expected_content = "SCRIPT_NAME = %s" % (SCRIPT_NAME)

    def Prepare (self, www):
        d = self.Mkdir (www, "script_name_1/sub/dir")
        self.WriteFile (d, "exec.cgi", 0755,
                        """#!/bin/sh

                        echo "Content-Type: text/plain"
                        echo
                        echo "SCRIPT_NAME = $SCRIPT_NAME"
                        """) 

        self.conf             = """
           Directory %s {
              Handler cgi
              DocumentRoot %s
           }""" % (VIRTUAL_DIR, d)
