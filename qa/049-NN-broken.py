from base import *

CONF = """
vserver!default!rule!490!match = directory
vserver!default!rule!490!match!directory = /missing
vserver!default!rule!490!handler = nn
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Broken NN"

        self.request          = "GET /missing/ HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 404

