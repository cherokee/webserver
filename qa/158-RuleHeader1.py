from qa_base import *

DIR       = "header_test1_referer_match"
REFERER   = "example.158ext"
MAGIC     = "Dealing with rule based headers.."

CONF = r"""
vserver!1!rule!1580!match = header
vserver!1!rule!1580!match!header = Referer
vserver!1!rule!1580!match!match = .+\.158ext
vserver!1!rule!1580!handler = cgi
"""

CGI = f"""#!/bin/sh

echo "Content-Type: text/plain"
echo
echo "{MAGIC}"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Rule header: match I"

        self.request           = f"GET /{DIR}/test HTTP/1.0\r\n" + \
                                 f"Referer: {REFERER}\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.required_content  = MAGIC
        self.forbidden_content = ["/bin/sh", "echo"]

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, 'test', 0o755, CGI)
