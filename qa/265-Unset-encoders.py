from base import *
from util import *

DIR = "test_265"

CONF = """
vserver!1!rule!2650!match = directory
vserver!1!rule!2650!match!directory = /%(DIR)s
vserver!1!rule!2650!encoder!gzip = 0
vserver!1!rule!2650!encoder!deflate = 0
"""

# Cherokee 1.0.11 failed to start whenever an encoder was unset (set
# to '0') rather than to 'allow', '1', or 'deny'.
#

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Encoders: all unset"
        self.request           = "GET /%(DIR)s/ HTTP/1.0\r\n" %(globals())
        self.conf              = CONF
        self.expected_error    = 200

    def Prepare (self, www):
        self.Mkdir (www, DIR)
