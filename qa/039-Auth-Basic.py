from base import *

CONF = """
vserver!1!rule!390!match = directory
vserver!1!rule!390!match!directory = /auth1
vserver!1!rule!390!match!final = 0
vserver!1!rule!390!auth = plain
vserver!1!rule!390!auth!methods = basic
vserver!1!rule!390!auth!realm = Test
vserver!1!rule!390!auth!passwdfile = %s
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
