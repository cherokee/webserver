from base import *

COMMENT = "This is comment inside the CGI"
TEXT    = "It should be printed by the CGI"

CONF = """
vserver!1!rule!1080!match = directory
vserver!1!rule!1080!match!directory = /prio2/sub
vserver!1!rule!1080!handler = file

vserver!1!rule!1081!match = extensions
vserver!1!rule!1081!match!extensions = prio2
vserver!1!rule!1081!handler = cgi
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Priorities: Dir and then Ext"

        self.request           = "GET /prio2/sub/exec.prio2 HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = TEXT
        self.forbidden_content = COMMENT
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, "prio2/sub")
        f = self.WriteFile (d, "exec.prio2", 0555,
                            """#!/bin/sh

                            echo "Content-type: text/html"
                            echo ""
                            # %s
                            echo "%s"
                            """ % (COMMENT, TEXT))
