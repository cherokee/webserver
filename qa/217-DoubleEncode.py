from base import *
from util import *

from cStringIO import StringIO
import gzip
from gzip import GzipFile

DIR    = "217_CGI_Encoded"
MAGIC  = "Random text follows: " + letters_random (64 * 1024)

CONF = """
vserver!1!rule!2170!encoder!gzip = 1
vserver!1!rule!2170!match = directory
vserver!1!rule!2170!match!directory = /%s
vserver!1!rule!2170!handler = cgi
""" % (DIR)

CGI_BASE = """#!/bin/sh
echo "Content-Type: text/plain"
echo "Content-Encoding: gzip"
echo
cat %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "GZip encoded CGI"
        self.request           = "GET /%s/file HTTP/1.0\r\n" % (DIR)+\
                                 "Accept-Encoding: gzip\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.forbidden_content = "Random text follows"

    def CustomTest (self):
        body_gz = self.reply[self.reply.find("\r\n\r\n")+4:]
        body = gzip.GzipFile('','r',0,StringIO(body_gz)).read()

        if body != MAGIC:
            return -1
        return 0

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)

        g = gzip.open('%s/temp.gz'%(d), 'wb')
        g.write(MAGIC)
        g.close()

        self.WriteFile (d, "file", 0755, CGI_BASE%("%s/temp.gz"%(d)))
