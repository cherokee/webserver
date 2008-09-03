import os
import pwd
from base import *

DIR            = "chunked_cgi_1"
MAGIC          = letters_random (300)
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
echo "%s"
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Chunked encoding: cgi"

        self.request           = "GET /%s/test HTTP/1.1\r\n" % (DIR) + \
                                 "Host: localhost\r\n"
        self.expected_error    = 200
        self.expected_content  = [MAGIC, "Transfer-Encoding: chunked", CHUNKED_FINISH]
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR, 0777)
        self.WriteFile (d, "test", 0555, CGI_CODE)
