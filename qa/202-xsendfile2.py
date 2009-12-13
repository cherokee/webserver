from base import *

DIR     = "DirXsendfile2"
FILE    = "test.cgi"
MAGIC   = str_random(100)
DISCART = "This text should be discarted"
HDR_NAM1 = "X-Foo"
HDR_VAL1 = "0"
HDR_NAM2 = "X-Example"
HDR_VAL2 = "This is a Long Value"

CONF = """
vserver!1!rule!2020!match = directory
vserver!1!rule!2020!match!directory = /%s
vserver!1!rule!2020!handler = cgi
vserver!1!rule!2020!handler!xsendfile = 1
"""

CGI = """#!/bin/sh

echo "Content-Type: text/plain"
echo "%s: %s"
echo "X-Sendfile: %s"
echo "%s: %s"
echo
echo "%s"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "X-Sendfile: File headers"

        self.request           = "GET /%s/%s HTTP/1.1\r\n" % (DIR, FILE) + \
                                 "Host: default\r\n"                     + \
                                 "Connection: Close\r\n"

        self.expected_error    = 200
        self.expected_content  = ["Content-Length: ", "ETag: ", "Last-Modified: ", MAGIC,
                                  "%s: %s\r\n" %(HDR_NAM1, HDR_VAL1),
                                  "%s: %s\r\n" %(HDR_NAM2, HDR_VAL2)]
        self.forbidden_content = ["/bin/sh", "echo", DISCART, "chunked"]
        self.conf              = CONF % (DIR)

    def Prepare (self, www):
        temp = self.WriteTemp (MAGIC)
        cgi  = CGI % (HDR_NAM1, HDR_VAL1, temp, HDR_NAM2, HDR_VAL2, DISCART)

        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, FILE, 0755, cgi)
