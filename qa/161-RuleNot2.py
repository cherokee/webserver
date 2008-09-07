from base import *

VSERVER = "rulenot2"
MAGIC   = "Just a not rule test"
DIR     = "DIR1"

CONF = """
vserver!1610!nick = %s
vserver!1610!document_root = %s

vserver!1610!rule!10!match = default
vserver!1610!rule!10!handler = file

vserver!1610!rule!20!match = not
vserver!1610!rule!20!match!right = directory
vserver!1610!rule!20!match!right!directory = /this_is_not
vserver!1610!rule!20!handler = cgi
"""

CGI = """#!/bin/sh

echo "Content-Type: text/plain"
echo 
echo "%s"
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Rule not: match"

        self.request           = "GET /%s/test HTTP/1.1\r\n" % (DIR) + \
                                 "Host: %s\r\n" % (VSERVER)          + \
                                 "Connection: Close\r\n"

        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["/bin/sh", "echo"]

    def Prepare (self, www):
        d = self.Mkdir (www, VSERVER)
        d2 = self.Mkdir (d, DIR)

        self.conf = CONF % (VSERVER, d)

        f = self.WriteFile (d2, 'test', 0755, CGI)
