import os.path
from qa_base import *

DIR = "211_empty_gif"

CONF = f"""
vserver!1!rule!2110!match = directory
vserver!1!rule!2110!match!directory = /{DIR}
vserver!1!rule!2110!handler = empty_gif
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Empty GIF"

        self.request          = f"GET /{DIR}/foo HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = ["Content-Type: image/gif",
                                 "GIF89a", "\x4c\x01\x00\x3B"]
        self.conf             = CONF
