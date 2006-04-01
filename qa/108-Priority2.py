from base import *

COMMENT = "This is comment inside the CGI"
TEXT    = "It should be printed by the CGI"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Priorities: Dir and then Ext"

        self.request           = "GET /prio2/sub/exec.prio2 HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = TEXT
        self.forbidden_content = COMMENT

        self.conf              = """Directory /prio2/sub {
                                      Handler file
                                    }
                                    Extension prio2 {
                                      Handler cgi
                                    }
                                 """

    def Prepare (self, www):
        d = self.Mkdir (www, "prio2/sub")
        f = self.WriteFile (d, "exec.prio2", 0555,
                            """#!/bin/sh

                            echo "Content-type: text/html"
                            echo ""
                            # %s
                            echo "%s"
                            """ % (COMMENT, TEXT))
