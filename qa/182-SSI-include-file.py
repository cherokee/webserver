from qa_base import *

DIR   = "ssi1"
MAGIC = '<a href="http://www.alobbs.com/">Alvaro</a>'
INC   = "test_182.inc"
FILE  = "example.shtml"

CONF = f"""
vserver!1!rule!1820!match = directory
vserver!1!rule!1820!match!directory = /{DIR}
vserver!1!rule!1820!handler = ssi
"""

FILE_CONTENT = f"""
<html>
  <head>
    <title>Cherokee QA Test</title>
    <!-- example -->
  </head>
  <body>
    <!--#include file="{INC}" -->
  </body>
</html>
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "SSI: include file"
        self.request           = f"GET /{DIR}/{FILE} HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0o444, FILE_CONTENT)
        self.WriteFile (d, INC,  0o444, MAGIC)
