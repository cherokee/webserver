from base import *

COMMENT = "This is comment inside the CGI"
TEXT    = "It should be printed by the CGI"

CONF = """
vserver!1!rule!1070!match = directory
vserver!1!rule!1070!match!directory = /prio1
vserver!1!rule!1070!handler = file

vserver!1!rule!1071!match = directory
vserver!1!rule!1071!match!directory = /prio1/sub
vserver!1!rule!1071!handler = cgi
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Priorities: Dir and then Dir"

        self.request           = "GET /prio1/sub/exec.cgi HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = TEXT
        self.forbidden_content = COMMENT
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, "prio1/sub")
        f = self.WriteFile (d, "exec.cgi", 0555,
                            """#!/bin/sh

                            echo "Content-type: text/html"
                            echo ""
                            # %s
                            echo "%s"
                            """ % (COMMENT, TEXT))
