from base import *

MAGIC="Alvaro: http://www.alobbs.com/"

PAIR1 = ("230/dir/2/three", "filename.txt")
PAIR2 = ("",                "230filename")

CONF = """
vserver!1!rule!2300!match = fullpath
vserver!1!rule!2300!match!fullpath!1 = /%s/%s
vserver!1!rule!2300!match!fullpath!2 = /%s
vserver!1!rule!2300!handler = cgi
"""

CGI_BASE = """#!/bin/sh
echo "Content-type: text/html"
echo ""
echo "%s"
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "FullPath: second path + query string"
        self.request           = "GET /%s?query=string HTTP/1.0\r\n" % (PAIR2[1])
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["/bin/sh", "echo"]
        self.conf              = CONF % (PAIR1[0], PAIR1[1], PAIR2[1])

    def Prepare (self, www):
        for pair in [PAIR1, PAIR2]:
            pd,pf = pair

            if pd:
                d = self.Mkdir (www, pd)
            else:
                d = www
            self.WriteFile (d, pf, 0755, CGI_BASE)

