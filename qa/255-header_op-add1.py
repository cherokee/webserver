from base import *

DIR    = "headerop_add1"
HEADER = "lalelilolu"
VALUE  = "barfoo"

CONF = """
vserver!1!rule!2550!match = directory
vserver!1!rule!2550!match!directory = /%(DIR)s
vserver!1!rule!2550!handler = dirlist
vserver!1!rule!2550!header_op!1!type = add
vserver!1!rule!2550!header_op!1!header = %(HEADER)s
vserver!1!rule!2550!header_op!1!value = %(VALUE)s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name           = "Header Ops: Add header"
        self.request        = "GET /%s/ HTTP/1.0\r\n" %(DIR)
        self.expected_error = 200
        self.conf           = CONF%(globals())

    def CustomTest (self):
        header = self.reply[:self.reply.find("\r\n\r\n")+2]
        if not "%(HEADER)s: %(VALUE)s\r\n" %(globals()) in header:
            return -1
        return 0

    def Prepare (self, www):
        self.Mkdir (www, DIR)
