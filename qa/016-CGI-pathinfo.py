from base import *

MAGIC = "It works"
CONF = """
vserver!1!rule!160!match = directory
vserver!1!rule!160!match!directory = /cgi-bin2
vserver!1!rule!160!handler = cgi
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "CGI with pathinfo"

        self.request          = "GET /cgi-bin2/test/parameter HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_content = MAGIC
        self.expected_error   = 200

    def Prepare (self, www):
        self.Mkdir (www, "cgi-bin2")
        self.WriteFile (www, "cgi-bin2/test", 0755,
                        """#!/bin/sh

                        echo "Content-Type: text/plain"
                        echo
                        echo "%s"
                        """ % (MAGIC))
