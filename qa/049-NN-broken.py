from base import *

CONF = """
vserver!default!directory!/missing!handler = nn
vserver!default!directory!/missing!priority = 490
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Broken NN"

        self.request          = "GET /missing/ HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 404

