from base import *

MAGIC = "Cherokee is pure magic"

CONF = """
vserver!0940!nick = request1host
vserver!0940!document_root = %s
vserver!0940!match = wildcard
vserver!0940!match!domain!1 = request1host

vserver!0940!rule!1!match = default
vserver!0940!rule!1!handler = server_info
vserver!0940!rule!10!match = directory
vserver!0940!rule!10!match!directory = /urlrequest1
vserver!0940!rule!10!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "URL request I"
        self.request          = "GET http://request1host/urlrequest1/dir/file HTTP/1.1\r\n" + \
                                "Connection: Close\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, "urlrequest1/dir")
        self.WriteFile (d, "file", 0444, MAGIC)

        self.conf = CONF % (www)
