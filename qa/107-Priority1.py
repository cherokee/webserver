from base import *

COMMENT = "This is comment inside the CGI"
TEXT    = "It should be printed by the CGI"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Priorities: Dir and then Dir"

        self.request           = "GET /prio1/sub/exec.cgi HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = TEXT
        self.forbidden_content = COMMENT

        self.conf              = """Directory /prio1/sub {
                                      Handler cgi
                                    }
                                    Directory /prio1 {
                                      Handler file
                                    }
                                 """

    def Prepare (self, www):
        d = self.Mkdir (www, "prio1/sub")
        f = self.WriteFile (d, "exec.cgi", 0555,
                            """#!/bin/sh

                            echo "Content-type: text/html"
                            echo ""
                            # %s
                            echo "%s"
                            """ % (COMMENT, TEXT))
