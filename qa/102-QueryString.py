from qa_base import *

DIR    = "querystring"
PARAMS = "param1=one&param2=two&param3"

CONF = """
vserver!1!rule!1020!match = directory
vserver!1!rule!1020!match!directory = /querystring
vserver!1!rule!1020!handler = cgi
"""

CGI_BASE = """#!/bin/sh
# %s
echo "Content-Type: text/plain"
echo
echo "QUERY_STRING = $QUERY_STRING"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "CGI: QUERY_STRING"

        self.request          = f"GET /{DIR}/exec.cgi?{PARAMS} HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 200
        self.expected_content = f"QUERY_STRING = {PARAMS}"

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "exec.cgi", 0o755, CGI_BASE)
