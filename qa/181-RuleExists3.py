from base import *

FILE      = 'special_file_for_181'
DIR       = '181_exists_dir'
MAGIC     = 'Alvaro__alobbs.com'
FORBIDDEN = 'This is forbidden string'

CONF = """
vserver!1!rule!1810!match = exists
vserver!1!rule!1810!match!exists = %s
vserver!1!rule!1810!match!final = 1
vserver!1!rule!1810!handler = file

vserver!1!rule!1811!match = directory
vserver!1!rule!1811!match!directory = %s
vserver!1!rule!1811!match!final = 1
vserver!1!rule!1811!handler = cgi
""" % (FILE, DIR)

CGI_BASE = """#!/bin/sh
echo "Content-type: text/html"
echo ""
cat << EOF
%s
EOF
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Rule Exists: mismatch"
        self.request           = "GET /%s/ HTTP/1.0\r\n" % (DIR)
        self.forbidden_content = ['/bin/sh', 'echo', FORBIDDEN]
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, MAGIC)
        self.WriteFile (www, FILE, 0555, CGI_BASE % (FORBIDDEN))

