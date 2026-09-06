from qa_base import *

DIR   = "ssi-rec1"
MAGIC = '<a href="http://www.alobbs.com/">Alvaro</a>'

CONF = f"""
vserver!1!rule!2840!match = directory
vserver!1!rule!2840!match!directory = /{DIR}
vserver!1!rule!2840!handler = ssi
"""

FILE = "example.shtml"
INC1 = "test_284.inc"
INC2 = "test_284.inc.part2"

FILE1_CONTENT = f"""
<html>
  <head>
    <title>Cherokee QA Test</title>
    <!-- example -->
  </head>
  <body>
    <!--#include file="{INC1}" -->
  </body>
</html>
"""

FILE2_CONTENT = f"""
<div>pre</div>
  <!--#include file="{INC2}" -->
<div>post</div>
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "SSI: recursive include file"
        self.request           = f"GET /{DIR}/{FILE} HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0o444, FILE1_CONTENT)
        self.WriteFile (d, INC1, 0o444, FILE2_CONTENT)
        self.WriteFile (d, INC2, 0o444, MAGIC)
