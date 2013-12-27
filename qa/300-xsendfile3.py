from base import *

DIR     = "DirXsendfile3"
FILE    = "test.cgi"
MAGIC   = str_random (100)
LENGTH  = 100
OFFSET  = 15
DISCART = "This text should be discarted"

CONF = """
vserver!1!rule!3000!match = directory
vserver!1!rule!3000!match!directory = /%s
vserver!1!rule!3000!handler = cgi
vserver!1!rule!3000!handler!xsendfile = 1
"""

CGI = """#!/bin/sh

echo "Content-Type: text/plain"
echo "X-Sendfile: %s"
echo
echo "%s"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "X-Sendfile: CGI (Range)"

        self.request           = "GET /%s/%s HTTP/1.1\r\n" % (DIR, FILE) + \
                                 "Host: default\r\n"                     + \
                                 "Range: bytes=%d-\r\n" % (OFFSET)       + \
                                 "Connection: Close\r\n"

        length = LENGTH - OFFSET
        self.expected_error    = 206
        self.expected_content  = [MAGIC[OFFSET:], "Content-Length: %d" % (length)]
        self.forbidden_content = ["/bin/sh", "echo", DISCART, MAGIC[:OFFSET]]
        self.conf              = CONF % (DIR)

    def Prepare (self, www):
        temp = self.WriteTemp (MAGIC)
        cgi  = CGI % (temp, DISCART)

        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, FILE, 0755, cgi)
