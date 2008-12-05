import os
from base import *

DIR   = "post_zero_len1"
MAGIC = '<a href="http://www.alobbs.com/">Alvaro</a>'

CONF  = """
vserver!1!rule!300!match = directory
vserver!1!rule!300!match!directory = /%s
vserver!1!rule!300!handler = cgi
""" % (DIR)

CGI_BASE = """#!/bin/sh
echo "Content-Type: text/plain"
echo
echo '%s'
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Post with length zero"

        self.request          = "POST /%s/test HTTP/1.0\r\n" % (DIR)+\
                                "Content-type: application/x-www-form-urlencoded\r\n" +\
                                "Content-length: 0\r\n"
        self.post             = ""
        self.conf             = CONF
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test", 0755, CGI_BASE)

