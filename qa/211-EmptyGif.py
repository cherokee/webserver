import os.path
from base import *

DIR = "211_empty_gif"

CONF = """
vserver!1!rule!2110!match = directory
vserver!1!rule!2110!match!directory = /%s
vserver!1!rule!2110!handler = empty_gif
""" % (DIR)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Empty GIF"

        self.request          = "GET /%s/foo HTTP/1.0\r\n" % (DIR)
        self.expected_error   = 200
        self.expected_content = ["Content-Type: image/gif",
                                 "GIF89a", "\x4c\x01\x00\x3B"]
        self.conf             = CONF

