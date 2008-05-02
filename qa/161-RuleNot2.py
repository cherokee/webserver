from base import *

VSERVER = "rulenot2"
MAGIC   = "Just a not rule test"
DIR     = "DIR1"

CONF = """
vserver!<host>!document_root = %s

vserver!<host>!rule!10!match = default
vserver!<host>!rule!10!handler = file

vserver!<host>!rule!20!match = not
vserver!<host>!rule!20!match!right = directory
vserver!<host>!rule!20!match!right!directory = /this_is_not
vserver!<host>!rule!20!handler = cgi
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
                                 "Host: %s\r\n" % (VSERVER)
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["/bin/sh", "echo"]

    def Prepare (self, www):
        d = self.Mkdir (www, VSERVER)
        d2 = self.Mkdir (d, DIR)

        tmp = CONF % (d)
        tmp = tmp.replace ('<host>', VSERVER)
        self.conf = tmp

        f = self.WriteFile (d2, 'test', 0755, CGI)
