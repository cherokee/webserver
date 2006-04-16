from base import *

CONF = """
vserver!default!directory!/auth_header!auth = plain
vserver!default!directory!/auth_header!auth!methods = basic
vserver!default!directory!/auth_header!auth!realm = Test
vserver!default!directory!/auth_header!auth!passwdfile = %s
vserver!default!directory!/auth_header!priority = 820
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Auth basic"
        self.request          = "GET /auth_header/ HTTP/1.0\r\n"
        self.expected_error   = 401
        self.expected_content = ["WWW-Authenticate:", "Basic realm=", "Test" ]
        self.conf             = CONF
        
        self.conf2             = """Directory /auth_header {
                                     Handler common
                                     Auth Basic {
                                          Name "Test"
                                          Method plain { PasswdFile /tmp/no_exists }
                                     }
                                }"""
    def Prepare (self, www):
        dir = self.Mkdir (www, "auth_header")
