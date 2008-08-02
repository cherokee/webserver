from base import *

CONF = """
vserver!001!rule!850!match = directory
vserver!001!rule!850!match!directory = /auth1users
vserver!001!rule!850!match!final = 0
vserver!001!rule!850!auth = plain
vserver!001!rule!850!auth!methods = basic
vserver!001!rule!850!auth!realm = Test with Users
vserver!001!rule!850!auth!passwdfile = %s
vserver!001!rule!850!auth!users = foo,alo
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Auth basic, with users"
        self.request          = "GET /auth1users/ HTTP/1.0\r\n"
        self.expected_error   = 401

    def Prepare (self, www):
        d = self.Mkdir (www, "auth1users")
        self.WriteFile (d, "passwd", 0444, 'user:cherokee\n' + 'foo:bar\n' + 'sending:sos\n')
        
        self.conf = CONF % (d+"/passwd")
