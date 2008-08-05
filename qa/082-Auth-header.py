from base import *

CONF = """
vserver!1!rule!820!match = directory
vserver!1!rule!820!match!directory = /auth_header
vserver!1!rule!820!match!final = 0
vserver!1!rule!820!auth = plain
vserver!1!rule!820!auth = plain
vserver!1!rule!820!auth!methods = basic
vserver!1!rule!820!auth!realm = Test
vserver!1!rule!820!auth!passwdfile = %s
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
