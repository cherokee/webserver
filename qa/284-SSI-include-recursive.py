from base import *

DIR   = "ssi-rec1"
MAGIC = '<a href="http://www.alobbs.com/">Alvaro</a>'

CONF = """
vserver!1!rule!2840!match = directory
vserver!1!rule!2840!match!directory = /%s
vserver!1!rule!2840!handler = ssi
""" % (DIR)

FILE = "example.shtml"
INC1 = "test_284.inc"
INC2 = "test_284.inc.part2"

FILE1_CONTENT = """
<html>
  <head>
    <title>Cherokee QA Test</title>
    <!-- example -->
  </head>
  <body>
    <!--#include file="%s" -->
  </body>
</html>
""" % (INC1)

FILE2_CONTENT = """
<div>pre</div>
  <!--#include file="%s" -->
<div>post</div>
""" % (INC2)


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "SSI: recursive include file"
        self.request           = "GET /%s/%s HTTP/1.0\r\n"%(DIR, FILE)
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0444, FILE1_CONTENT)
        self.WriteFile (d, INC1, 0444, FILE2_CONTENT)
        self.WriteFile (d, INC2, 0444, MAGIC)

