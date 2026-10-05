from qa_base import *

DIR   = "header_arg_match_all_2"
ARGS  = "first238=1&second238=http://1.1.1.1&third238=3"
MAGIC = "magic string"

CONF = f"""
vserver!1!rule!2380!match = and
vserver!1!rule!2380!match!left = url_arg
vserver!1!rule!2380!match!left!match = not_found
vserver!1!rule!2380!match!right = directory
vserver!1!rule!2380!match!right!directory = /{DIR}
vserver!1!rule!2380!handler = cgi
"""

CGI = f"""#!/bin/sh

echo "Content-Type: text/plain"
echo
echo "{MAGIC}"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Rule url_arg: no match all"

        self.request           = f"GET /{DIR}/test?{ARGS} HTTP/1.0\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.required_content  = ["/bin/sh", "echo", MAGIC]

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, 'test', 0o755, CGI)
