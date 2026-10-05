from qa_base import *

DIR = "cgi-bin_serveraddr"

CONF = f"""
vserver!1!rule!1950!match = directory
vserver!1!rule!1950!match!directory = /{DIR}
vserver!1!rule!1950!handler = cgi
"""

CGI_BASE = """#!/bin/sh
echo "Content-Type: text/plain"
echo
echo "SERVER_ADDR: $SERVER_ADDR"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "CGI: SERVER_ADDR"

        self.request        = f"GET /{DIR}/test HTTP/1.0\r\n"
        self.conf           = CONF
        self.expected_error = 200
        self.proxy_suitable = False

    def CustomTest (self):
        body = self.reply.split ("\r\n\r\n")[1]

        if "127.0.0.1" in body:
            return 0
        if "::1" in body:
            return 0

        return -1

    def Prepare (self, www):
        self.Mkdir (www, DIR)
        self.WriteFile (www, f"{DIR}/test", 0o755, CGI_BASE)
