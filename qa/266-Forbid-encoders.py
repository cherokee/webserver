from base import *
from util import *

DIR  = "test_266"
FILE = "filename.txt"

CONF = """
vserver!1!rule!2650!match = directory
vserver!1!rule!2650!match!directory = /%(DIR)s
vserver!1!rule!2650!encoder!gzip = forbid
"""

# Cherokee 1.0.11 failed to start whenever an encoder was set to
# 'forbid'.
#

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Encoders: forbid"
        self.request           = "GET /%(DIR)s/ HTTP/1.0\r\n" %(globals()) + \
                                 "Accept-Encoding: gzip\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.expected_content  = [FILE]

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE)
