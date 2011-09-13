from base import *
from util import *

from cStringIO import StringIO
from gzip import GzipFile

WEB_DIR = "gzip_EI16_1"
MAGIC   = "Random text follows: " + str_random (10 * 1024)
AGENT   = "Mozilla/4.0 (compatible; MSIE 6.0; Windows 98; Win 9x 4.90)"

CONF = """
vserver!1!rule!2880!match = directory
vserver!1!rule!2880!match!directory = /%(WEB_DIR)s
vserver!1!rule!2880!handler = file
vserver!1!rule!2880!encoder!gzip = 1
""" %(globals())


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "GZip from IE [1-6]"

        self.request           = "GET /%(WEB_DIR)s/file.txt HTTP/1.0\r\n" %(globals()) +\
                                 "User-Agent: %(AGENT)s\r\n" %(globals()) + \
                                 "Accept-Encoding: gzip\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.expected_content  = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, WEB_DIR)
        self.WriteFile (d, "file.txt", 0444, MAGIC)

