from qa_base import *

DIR   = "header_arg2_match"
ARGS  = "first236=1&second236=http://1.1.1.1&third236=3"
MAGIC = "magic string"

CONF = """
vserver!1!rule!2360!match = url_arg
vserver!1!rule!2360!match!arg = second236
vserver!1!rule!2360!match!match = isnt_there
vserver!1!rule!2360!handler = cgi
"""

CGI = f"""#!/bin/sh

echo "Content-Type: text/plain"
echo
echo "{MAGIC}"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Rule url_arg: no match"

        self.request           = f"GET /{DIR}/test?{ARGS} HTTP/1.0\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.required_content  = ["/bin/sh", "echo", MAGIC]

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, 'test', 0o755, CGI)
