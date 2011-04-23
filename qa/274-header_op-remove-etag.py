from base import *

DIR   = "headerop_remove_etag"
MAGIC = "ETags are not so bad as some people say."

CONF = """
vserver!1!rule!2740!match = directory
vserver!1!rule!2740!match!directory = /%(DIR)s
vserver!1!rule!2740!handler = file
vserver!1!rule!2740!header_op!1!type = del
vserver!1!rule!2740!header_op!1!header = ETag
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Header Ops: Remove ETag header"
        self.request          = "GET /%s/file HTTP/1.1\r\n" %(DIR) + \
                                "Host: localhost\r\n" + \
                                "Connection: close\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC
        self.conf             = CONF%(globals())

    def CustomTest (self):
        header = self.reply[:self.reply.find("\r\n\r\n")+2]
        if "ETag"in header:
            return -1
        return 0

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "file", 0444, MAGIC)
