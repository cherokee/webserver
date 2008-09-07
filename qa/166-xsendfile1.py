from base import *

DIR     = "DirXsendfile1"
FILE    = "test.cgi"
MAGIC   = str_random (100)
DISCART = "This text should be discarted"

CONF = """
vserver!1!rule!1660!match = directory
vserver!1!rule!1660!match!directory = /%s
vserver!1!rule!1660!handler = cgi
vserver!1!rule!1660!handler!xsendfile = 1
"""

CGI = """#!/bin/sh

echo "Content-Type: text/plain"
echo "X-Sendfile: %s"
echo 
echo "%s"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "X-Sendfile: CGI"

        self.request           = "GET /%s/%s HTTP/1.1\r\n" % (DIR, FILE) + \
                                 "Host: default\r\n"                     + \
                                 "Connection: Close\r\n"

        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["/bin/sh", "echo", DISCART]
        self.conf              = CONF % (DIR)

    def Prepare (self, www):
        temp = self.WriteTemp (MAGIC)
        cgi  = CGI % (temp, DISCART)
        
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, FILE, 0755, cgi)
