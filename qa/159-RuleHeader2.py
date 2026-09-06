from qa_base import *

DIR       = "header_test2_referer_match"
REFERER   = "example.159com"
MAGIC     = "Dealing with rule based headers (bis).."

CONF = r"""
vserver!1!rule!1590!match = header
vserver!1!rule!1590!match!header = Referer
        vserver!1!rule!1590!match!match = .+\.159com
vserver!1!rule!1590!handler = file

vserver!1!rule!1591!match = header
vserver!1!rule!1591!match!header = Referer
vserver!1!rule!1591!match!match = .+\.159net
vserver!1!rule!1591!handler = cgi
"""

CGI = f"""#!/bin/sh

echo "Content-Type: text/plain"
echo
echo "{MAGIC}"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Rule header: match II"

        self.request           = f"GET /{DIR}/test HTTP/1.0\r\n" + \
                                 f"Referer: {REFERER}\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.required_content  = ["/bin/sh", "echo"]

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, 'test', 0o755, CGI)
