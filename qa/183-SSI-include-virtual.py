from qa_base import *

DIR   = "ssi2"
MAGIC = '<a href="http://www.alobbs.com/">Alvaro</a>'
INC   = "test_183.inc"
FILE  = "example.shtml"

CONF = f"""
vserver!1!rule!1830!match = directory
vserver!1!rule!1830!match!directory = /{DIR}
vserver!1!rule!1830!handler = ssi
"""

FILE_CONTENT = f"""
<html>
  <head>
    <title>Cherokee QA Test: SSI 'virtual' include</title>
    <!-- example -->
  </head>
  <body>
    <!--#include virtual="{INC}" -->
  </body>
</html>
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "SSI: include virtual"
        self.request           = f"GET /{DIR}/{FILE} HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d,  FILE, 0o444, FILE_CONTENT)
        self.WriteFile (www, INC, 0o444, MAGIC)
