from base import *

CONF = """
vserver!1!rule!3060!match = directory
vserver!1!rule!3060!match!directory = /cgi_error_204_1
vserver!1!rule!3060!handler = cgi
vserver!1!rule!3060!handler!error_handler = 1
"""

CGI_BASE = """#!/bin/sh
echo "Status: 204"
echo ""
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "204 No Content with Keep-Alive"
        self.expected_error = 204
        self.conf           = CONF

        request_base = "GET /cgi_error_204_1/exec.cgi HTTP/1.1\r\n" + \
                       "Host: example.com\r\n"

        self.request = request_base + \
                       "Connection: keep-alive\r\n" + \
                       "\r\n" + \
                       request_base + \
                       "Connection: close\r\n"

    def Prepare (self, www):
        d = self.Mkdir (www, "cgi_error_204_1")
        f = self.WriteFile (d, "exec.cgi", 0555, CGI_BASE )

    def CustomTest (self):

        parts = self.reply.split ('HTTP/1.1 204')
        if len(parts) != 3:
            return -1

        first_response  = parts[1]
        second_response = parts[2]

        if "close" in first_response.lower():
            return -1

        if not "close" in second_response.lower():
            return -1

        return 0

