# -*- coding: utf-8 -*-

import os
from base import *

DIR   = "dup-cache-control-1"
FILE  = "file"
EXT   = "test246"
MAGIC = "Alvaro: http://www.alobbs.com/"

CONF = """
vserver!1!rule!2460!match = directory
vserver!1!rule!2460!match!directory = /%s
vserver!1!rule!2460!match!final = 0
vserver!1!rule!2460!expiration = time
vserver!1!rule!2460!expiration!time = 2m

mime!application/xml!extensions = %s
mime!application/xml!max-age = 0
""" % (DIR, EXT)


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Dup Cache-Control: max-age"

        self.request          = "GET /%s/%s.%s HTTP/1.0\r\n" %(DIR, FILE, EXT)
        self.expected_error   = 200
        self.expected_content = MAGIC
        self.conf             = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "%s.%s"%(FILE,EXT), 0666, MAGIC)

    def CustomTest (self):
        body_low = self.reply.lower()

        if body_low.count("cache-control") != 1:
            return -1

        if body_low.count("max-age=") != 1:
            return -1

        return 0
