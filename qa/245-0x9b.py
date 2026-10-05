# -*- coding: utf-8 -*-

import os
from qa_base import *

DIR   = "0x9b_1"
FILE  = "国"
MAGIC = "0x9b is an ANSI escape.."

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "ANSI 0x9b in a UTF-8 request"
        self.request          = f"GET /{DIR}/{FILE} HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0o666, MAGIC)
