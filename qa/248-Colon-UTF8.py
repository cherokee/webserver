# -*- coding: utf-8 -*-

from qa_base import *

DIR  = "colon-utf8-1"
FILE = "a農:b園c"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "UTF8 file with colon"

        self.request          = f"GET /{DIR}/ HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = 'href="{}"'.format(FILE.replace(':','%3a'))

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0o444, "foo")
