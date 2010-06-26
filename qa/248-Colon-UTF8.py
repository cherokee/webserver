# -*- coding: utf-8 -*-

from base import *

DIR  = "colon-utf8-1"
FILE = "a農:b園c"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "UTF8 file with colon"

        self.request          = "GET /%s/ HTTP/1.0\r\n" % (DIR)
        self.expected_error   = 200
        self.expected_content = 'href="%s"' %(FILE.replace(':','%3a'))

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0444, "foo")

