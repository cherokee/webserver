from base import *

CONF = """
vserver!default!rule!820!match = directory
vserver!default!rule!820!match!directory = /auth_header
vserver!default!rule!820!match!final = 0
vserver!default!rule!820!auth = plain
vserver!default!rule!820!auth = plain
vserver!default!rule!820!auth!methods = basic
vserver!default!rule!820!auth!realm = Test
vserver!default!rule!820!auth!passwdfile = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Auth header"
        self.request          = "GET /auth_header/ HTTP/1.0\r\n"
        self.expected_error   = 401
        self.expected_content = ["WWW-Authenticate:", "Basic realm=", "Test" ]
        self.conf             = CONF

    def Prepare (self, www):
        dir = self.Mkdir (www, "auth_header")
