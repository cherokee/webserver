from qa_base import *

MAGIC   = "Rule OR: does not match"
DIR     = "DirOr2"
FILE    = "test.cgi"

CONF = """
vserver!1!rule!1650!match = directory
vserver!1!rule!1650!match!directory = /%s
vserver!1!rule!1650!handler = cgi

vserver!1!rule!1651!match = or
vserver!1!rule!1651!match!left = directory
vserver!1!rule!1651!match!left!directory = /other/dir
vserver!1!rule!1651!match!right = extensions
vserver!1!rule!1651!match!right!extensions = not_cgi,other
vserver!1!rule!1651!handler = file
"""

CGI = f"""#!/bin/sh

echo "Content-Type: text/plain"
echo
echo "{MAGIC}"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Rule or: not match"

        self.request           = f"GET /{DIR}/{FILE} HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["/bin/sh", "echo"]
        self.conf              = CONF % (DIR)

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, FILE, 0o755, CGI)
