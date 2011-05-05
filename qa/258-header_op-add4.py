import re
from base import *

DIR    = "headerop_add4"
HEADER = "X-header-check"
VALUE  = "1234567890ABCD"

CONF = """
vserver!1!rule!2580!match = directory
vserver!1!rule!2580!match!directory = /%(DIR)s
vserver!1!rule!2580!handler = dirlist
vserver!1!rule!2580!header_op!1!type = add
vserver!1!rule!2580!header_op!1!header = %(HEADER)s
vserver!1!rule!2580!header_op!1!value = %(VALUE)s
vserver!1!rule!2580!header_op!2!type = del
vserver!1!rule!2580!header_op!2!header = %(HEADER)s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name           = "Header Ops: Add header, multiple"
        self.request        = "GET /%s/ HTTP/1.0\r\n" %(DIR)
        self.expected_error = 200
        self.conf           = CONF%(globals())

    def CustomTest (self):
        header = self.reply[:self.reply.find("\r\n\r\n")+2]

        if HEADER in header:
            return -1
        if VALUE in header:
            return -1

        return 0

    def Prepare (self, www):
        self.Mkdir (www, DIR)
