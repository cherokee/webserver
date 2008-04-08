from base import *

CONF = """
vserver!default!rule!directory!/auth_header!auth = plain
vserver!default!rule!directory!/auth_header!auth!methods = basic
vserver!default!rule!directory!/auth_header!auth!realm = Test
vserver!default!rule!directory!/auth_header!auth!passwdfile = %s
vserver!default!rule!directory!/auth_header!priority = 820
vserver!default!rule!directory!/auth_header!final = 0
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
