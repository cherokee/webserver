# -*- coding: utf-8 -*-

import os
from qa_base import *

DIR   = "dup-cache-control-1"
FILE  = "file"
EXT   = "test246"
MAGIC = "Alvaro: http://www.alobbs.com/"

CONF = f"""
vserver!1!rule!2460!match = directory
vserver!1!rule!2460!match!directory = /{DIR}
vserver!1!rule!2460!match!final = 0
vserver!1!rule!2460!expiration = time
vserver!1!rule!2460!expiration!time = 2m

mime!application/xml!extensions = {EXT}
mime!application/xml!max-age = 0
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Dup Cache-Control: max-age"

        self.request          = f"GET /{DIR}/{FILE}.{EXT} HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC
        self.conf             = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, f"{FILE}.{EXT}", 0o666, MAGIC)

    def CustomTest (self):
        body_low = self.reply.lower()

        if body_low.count("cache-control") != 1:
            return -1

        if body_low.count("max-age=") != 1:
            return -1

        return 0
