# -*- coding: utf-8 -*-

from base import *

DIR      = "utf8file1"
FILENAME = "¡ĤĒĹĻŎ!"
MAGIC    = "So, Cherokee does support UTF8"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "UTF8 file download"

        self.request           = "GET /%s/%s HTTP/1.0\r\n" %(DIR, FILENAME)
        self.expected_error    = 200
        self.expected_content  = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, FILENAME, 0644, MAGIC)
