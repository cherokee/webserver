from base import *
from util import *

from cStringIO import StringIO
import zlib

PRE    = "Random text follows"
MAGIC  = PRE + ": " + str_random (10 * 1024)

CONF = """
vserver!1!rule!1680!match = directory
vserver!1!rule!1680!match!directory = /deflate1
vserver!1!rule!1680!handler = file
vserver!1!rule!1680!encoder!deflate = 1
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Deflate encoding 10k"

        self.request           = "GET /deflate1/file.txt HTTP/1.0\r\n" +\
                                 "Accept-Encoding: deflate\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.forbidden_content = PRE

    def CustomTest (self):
        # Get the server reply body
        body_gz = self.reply[self.reply.find("\r\n\r\n")+4:]

        # Decompress it
        body = zlib.decompress (body_gz, -zlib.MAX_WBITS)

        if body != MAGIC:
            return -1
        return 0

    def Prepare (self, www):
        self.Mkdir (www, "deflate1")
        self.WriteFile (www, "deflate1/file.txt", 0444, MAGIC)

