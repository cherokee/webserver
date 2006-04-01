from base import *

MAGIC = "It works"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "CGI with pathinfo"

        self.request          = "GET /cgi-bin2/test/parameter HTTP/1.0\r\n"
        self.conf             = "Directory /cgi-bin2 { Handler cgi }"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "cgi-bin2")
        self.WriteFile (www, "cgi-bin2/test", 0755,
                        """#!/bin/sh

                        echo "Content-Type: text/plain"
                        echo
                        echo "%s"
                        """ % (MAGIC))
