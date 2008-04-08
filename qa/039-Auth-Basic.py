from base import *

CONF = """
vserver!default!rule!directory!/auth1!auth = plain
vserver!default!rule!directory!/auth1!auth!methods = basic
vserver!default!rule!directory!/auth1!auth!realm = Test
vserver!default!rule!directory!/auth1!auth!passwdfile = %s
vserver!default!rule!directory!/auth1!final = 0
vserver!default!rule!directory!/auth1!priority = 390
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Auth basic"
        self.request          = "GET /auth1/ HTTP/1.0\r\n"
        self.expected_error   = 401

    def Prepare (self, www):
        d = self.Mkdir (www, "auth1")
        self.WriteFile (d, "passwd", 0444, 'user:cherokee')

        self.conf = CONF % (d+"/passwd")
