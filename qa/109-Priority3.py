from base import *

COMMENT = "This is comment inside the CGI"
TEXT    = "It should be printed by the CGI"

CONF = """
vserver!1!rule!1090!match = extensions
vserver!1!rule!1090!match!extensions = prio3
vserver!1!rule!1090!handler = file

vserver!1!rule!1091!match = directory
vserver!1!rule!1091!match!directory = /prio3/sub
vserver!1!rule!1091!handler = cgi
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Priorities: Ext and then Dir"

        self.request           = "GET /prio3/sub/exec.prio3 HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = TEXT
        self.forbidden_content = COMMENT
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, "prio3/sub")
        f = self.WriteFile (d, "exec.prio3", 0555,
                            """#!/bin/sh

                            echo "Content-type: text/html"
                            echo ""
                            # %s
                            echo "%s"
                            """ % (COMMENT, TEXT))
