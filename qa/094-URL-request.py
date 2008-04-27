from base import *

MAGIC = "Cherokee is pure magic"

CONF = """
vserver!request1host!document_root = %s
vserver!request1host!domain!1 = request1host

vserver!request1host!rule!1!match = default
vserver!request1host!rule!1!handler = server_info
vserver!request1host!rule!10!match = directory
vserver!request1host!rule!10!match!directory = /urlrequest1
vserver!request1host!rule!10!handler = file
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
