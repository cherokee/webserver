from base import *

CONF = """
vserver!default!directory!/auth1users!auth = plain
vserver!default!directory!/auth1users!auth!methods = basic
vserver!default!directory!/auth1users!auth!realm = Test with Users
vserver!default!directory!/auth1users!auth!passwdfile = %s
vserver!default!directory!/auth1users!auth!users = foo,alo
vserver!default!directory!/auth1users!priority = 850
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
