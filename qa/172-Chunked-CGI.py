from base import *

DIR            = "chunked_cgi_1"
MAGIC          = letters_random (300)

CHUNKED_BEGIN  = hex(len(MAGIC))[2:] + '\r\n'
CHUNKED_FINISH = "0\r\n\r\n"

CONF = """
vserver!1!rule!1720!match = directory
vserver!1!rule!1720!match!directory = /%s
vserver!1!rule!1720!handler = cgi
vserver!1!rule!1720!handler!allow_chunked = 1
""" % (DIR)

CGI_CODE = """#!/bin/sh

echo "Content-Type: text/plain"
echo

# echo adds a new line at the end, -n from bash
%s -c "import sys; sys.stdout.write('%s')"
""" % (look_for_python(), MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Chunked encoding: cgi"

        self.request           = "GET /%s/test HTTP/1.1\r\n" % (DIR) + \
                                 "Host: localhost\r\n"               + \
                                 "Connection: Keep-Alive\r\n\r\n"    + \
                                 "OPTIONS / HTTP/1.0\r\n"
                                 # Added another one to close the connection

        self.expected_error    = 200
        self.expected_content  = ["Transfer-Encoding: chunked",
                                  CHUNKED_BEGIN, MAGIC, CHUNKED_FINISH]
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR, 0777)
        self.WriteFile (d, "test", 0555, CGI_CODE)
