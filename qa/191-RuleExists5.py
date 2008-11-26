from base import *

HOST      = 'host191'
FILE      = 'special_file_for_191'
VSRV_DIR  = '191_vserver'
DIR       = 'what/ever'
MAGIC     = '<a href="http://www.alobbs.com/">Alvaro</a>'
FORBIDDEN = 'This is forbidden string'

CONF = """
vserver!191!nick = %s
vserver!191!document_root = %s

vserver!191!rule!3!match = exists
vserver!191!rule!3!match!match_any = 1
vserver!191!rule!3!match!final = 1
vserver!191!rule!3!handler = cgi

vserver!191!rule!2!match = directory
vserver!191!rule!2!match!directory = %s
vserver!191!rule!2!match!final = 1
vserver!191!rule!2!handler = file

vserver!191!rule!1!match = default
vserver!191!rule!1!match!final = 1
vserver!191!rule!1!handler = file
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
        self.name              = "Rule Exists: match any (in dir)"
        self.request           = "GET /%s/%s HTTP/1.0\r\n" % (DIR, FILE) +\
                                 "Host: %s\r\n" %(HOST)
        self.forbidden_content = ['/bin/sh', 'echo', FORBIDDEN]
        self.expected_error    = 200
        self.expected_content  = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, VSRV_DIR)
        self.conf = CONF % (HOST, d, DIR)

        e = self.Mkdir (d, DIR)
        self.WriteFile (e, FILE, 0555, CGI_BASE%(FORBIDDEN, MAGIC))

