from base import *
from util import *

from cStringIO import StringIO
from gzip import GzipFile


MAGIC  = "Random text follows: " + str_random (10 * 1024)

CONF = """
vserver!1!rule!890!match = directory
vserver!1!rule!890!match!directory = /gzip1
vserver!1!rule!890!handler = file
vserver!1!rule!890!encoder!gzip = 1
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "GZip encoding 10k"

        self.request           = "GET /gzip1/file.txt HTTP/1.0\r\n" +\
                                 "Accept-Encoding: gzip\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.forbidden_content = "Random text follows"

    def CustomTest (self):
        body_gz = self.reply[self.reply.find("\r\n\r\n")+4:]
        body = GzipFile('','r',0,StringIO(body_gz)).read()

        if body != MAGIC:
            return -1
        return 0

    def Prepare (self, www):
        self.Mkdir (www, "gzip1")
        self.WriteFile (www, "gzip1/file.txt", 0444, MAGIC)

