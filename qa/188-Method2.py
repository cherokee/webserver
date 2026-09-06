from qa_base import *

DIR       = 'method_rule_2'
MAGIC     = '<a href="http://www.alobbs.com/">Alvaro</a>'
FORBIDDEN = 'This is forbidden string'
METHOD    = 'post'

CONF = f"""
vserver!1!rule!1881!match = and
vserver!1!rule!1881!match!left = method
vserver!1!rule!1881!match!left!method = {METHOD}
vserver!1!rule!1881!match!right = directory
vserver!1!rule!1881!match!right!directory = /{DIR}
vserver!1!rule!1881!match!final = 1
vserver!1!rule!1881!handler = file

vserver!1!rule!1880!match = directory
vserver!1!rule!1880!match!directory = /{DIR}
vserver!1!rule!1880!match!final = 1
vserver!1!rule!1880!handler = cgi
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
        self.name              = "Rule Method: doesn't match"
        self.request           = f"GET /{DIR}/test HTTP/1.0\r\n"
        self.expected_content  = MAGIC
        self.forbidden_content = ['/bin/sh', 'echo', FORBIDDEN]
        self.expected_error    = 200
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test", 0o555, CGI_BASE % (FORBIDDEN, MAGIC))
