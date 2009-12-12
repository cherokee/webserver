# -*- coding: utf-8 -*-

from base import *

DIR      = "utf8-pathinfo1"
PATHINFO = "/ḈḨeṜṒḲḚḜ/supports/ṷẗḟ8"

CONF = """
vserver!1!rule!2410!match = directory
vserver!1!rule!2410!match!directory = /%s
vserver!1!rule!2410!handler = cgi
""" %(DIR)

CGI_BASE = """#!/bin/sh
echo "Content-Type: text/plain"
echo
echo "PathInfo is $PATH_INFO"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "UTF8 pathinfo"

        self.request          = "GET /%s/test%s HTTP/1.0\r\n" %(DIR, PATHINFO)
        self.conf             = CONF
        self.expected_error   = 200
        self.expected_content = "PathInfo is %s" % (PATHINFO)

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test", 0755, CGI_BASE)
