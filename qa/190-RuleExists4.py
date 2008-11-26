from base import *

HOST      = 'host190'
FILE      = 'special_file_for_190'
DIR       = '190_exists'
MAGIC     = '<a href="http://www.alobbs.com/">Alvaro</a>'
FORBIDDEN = 'This is forbidden string'

CONF = """
vserver!190!nick = %s
vserver!190!document_root = %s

vserver!190!rule!2!match = exists
vserver!190!rule!2!match!match_any = 1
vserver!190!rule!2!match!final = 1
vserver!190!rule!2!handler = cgi

vserver!190!rule!1!match = default
vserver!190!rule!1!match!final = 1
vserver!190!rule!1!handler = file
"""

CGI_BASE = """#!/bin/sh
echo "Content-type: text/html"
echo ""
# %s
cat << EOF
%s
EOF
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Rule Exists: match any"
        self.request           = "GET /%s HTTP/1.0\r\n" % (FILE) +\
                                 "Host: %s\r\n" %(HOST)
        self.forbidden_content = ['/bin/sh', 'echo', FORBIDDEN]
        self.expected_error    = 200
        self.expected_content  = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.conf = CONF % (HOST, d)

        self.WriteFile (d, FILE, 0555, CGI_BASE%(FORBIDDEN, MAGIC))

