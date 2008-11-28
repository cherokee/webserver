from base import *

HOST  = "192_test"
DIR   = "192_dir"
FILE  = "test_file"
MAGIC = "something"

CONF = """
vserver!192!nick = %s
vserver!192!document_root = %s

vserver!192!rule!300!match = request
vserver!192!rule!300!match!final = 0
vserver!192!rule!300!match!request = /%s/(.*)$
vserver!192!rule!300!handler = redir
vserver!192!rule!300!handler!rewrite!1!show = 0
vserver!192!rule!300!handler!rewrite!1!substring = /cgi-bin/program.cgi?param=$1

vserver!192!rule!200!match = directory
vserver!192!rule!200!match!directory = /cgi-bin
vserver!192!rule!200!match!final = 1
vserver!192!rule!200!handler = cgi
vserver!192!rule!200!document_root = %s

vserver!192!rule!100!match = default
vserver!192!rule!100!match!final = 1
vserver!192!rule!100!handler = file
"""

CGI_BASE = """#!/bin/sh
echo "Content-type: text/html"
echo
echo "%s"
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Hiden Redir: DocumentRoot change"
        self.request           = "GET /%s/%s HTTP/1.0\r\n" % (DIR, FILE) + \
                                 "Host: %s\r\n" % (HOST)
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = "/bin/sh"

    def Prepare (self, www):
        d = self.Mkdir (www, HOST)
        c = self.Mkdir (d, "cgi-bin_root")
        self.WriteFile (c, "program.cgi", 0555, CGI_BASE)

	self.conf = CONF % (HOST, d, DIR, c)


