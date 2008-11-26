from base import *

DIR       = 'method_rule_1'
MAGIC     = '<a href="http://www.alobbs.com/">Alvaro</a>'
FORBIDDEN = 'This is forbidden string'
METHOD    = 'get'

CONF = """
vserver!1!rule!1871!match = and
vserver!1!rule!1871!match!left = method
vserver!1!rule!1871!match!left!method = %s
vserver!1!rule!1871!match!right = directory
vserver!1!rule!1871!match!right!directory = /%s
vserver!1!rule!1871!match!final = 1
vserver!1!rule!1871!handler = cgi

vserver!1!rule!1870!match = directory
vserver!1!rule!1870!match!directory = /%s
vserver!1!rule!1870!match!final = 1
vserver!1!rule!1870!handler = file
""" % (METHOD, DIR, DIR)

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
        self.name              = "Rule Method: match"
        self.request           = "GET /%s/test HTTP/1.0\r\n" % (DIR)
        self.expected_content  = MAGIC
        self.forbidden_content = ['/bin/sh', 'echo', FORBIDDEN]
        self.expected_error    = 200
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test", 0555, CGI_BASE % (FORBIDDEN, MAGIC))
