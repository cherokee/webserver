from base import *

MAGIC = "Cherokee is pure magic"

CONF = """
vserver!request1host!document_root = %s
vserver!request1host!domain!1 = request1host
vserver!request1host!directory!/urlrequest1!handler = file
vserver!request1host!directory!/urlrequest1!priority = 10
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "URL request I"
        self.request          = "GET http://request1host/urlrequest1/dir/file HTTP/1.1\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, "urlrequest1/dir")
        self.WriteFile (d, "file", 0444, MAGIC)

        self.conf = CONF % (www)
