from qa_base import *

MAGIC   = "Rule AND: doesn't match"
DIR     = "DirAnd2"
FILE    = "test.cgi"

CONF = """
vserver!1!rule!1630!match = directory
vserver!1!rule!1630!match!directory = /%s
vserver!1!rule!1630!handler = cgi

vserver!1!rule!1631!match = and
vserver!1!rule!1631!match!left = directory
vserver!1!rule!1631!match!left!directory = /%s
vserver!1!rule!1631!match!right = extensions
vserver!1!rule!1631!match!right!extensions = not_cgi,other
vserver!1!rule!1631!handler = file
"""

CGI = f"""#!/bin/sh

echo "Content-Type: text/plain"
echo
echo "{MAGIC}"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Rule and: not match"

        self.request           = f"GET /{DIR}/{FILE} HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["/bin/sh", "echo"]
        self.conf              = CONF % (DIR, DIR)

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, FILE, 0o755, CGI)
