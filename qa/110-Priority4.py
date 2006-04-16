from base import *

COMMENT = "This is comment inside the CGI"
TEXT    = "It should be printed by the CGI"

CONF = """
vserver!default!directory!/request_entry1!handler = file
vserver!default!directory!/request_entry1!priority = 1100

vserver!default!request!request_entry1.*cgi$!handler = cgi
vserver!default!request!request_entry1.*cgi$!priority = 1101
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Priorities: Dir and then Req"

        self.request           = "GET /request_entry1/sub/exec.cgi HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = TEXT
        self.forbidden_content = COMMENT
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, "request_entry1/sub")
        f = self.WriteFile (d, "exec.cgi", 0555,
                            """#!/bin/sh

                            echo "Content-type: text/html"
                            echo ""
                            # %s
                            echo "%s"
                            """ % (COMMENT, TEXT))
