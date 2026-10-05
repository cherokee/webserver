from qa_base import *
from qa_util import *

from io import BytesIO
import gzip
from gzip import GzipFile

DIR    = "217_CGI_Encoded"
MAGIC  = "Random text follows: " + letters_random (64 * 1024)

CONF = f"""
vserver!1!rule!2170!encoder!gzip = 1
vserver!1!rule!2170!match = directory
vserver!1!rule!2170!match!directory = /{DIR}
vserver!1!rule!2170!handler = cgi
"""

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
        self.request           = f"GET /{DIR}/file HTTP/1.0\r\n"+\
                                 "Accept-Encoding: gzip\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.forbidden_content = "Random text follows"

    def CustomTest (self):
        body_gz = self.reply[self.reply.find("\r\n\r\n")+4:]
        body = gzip.GzipFile(fileobj=BytesIO(body_gz.encode("latin-1")), mode='r').read().decode()

        if body != MAGIC:
            return -1
        return 0

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)

        g = gzip.open(f'{d}/temp.gz', 'wb')
        g.write(MAGIC)
        g.close()

        self.WriteFile (d, "file", 0o755, CGI_BASE%(f"{d}/temp.gz"))
