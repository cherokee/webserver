from base import *

MAGIC  = "Don't show this"

CONF = """
vserver!1!rule!900!match = directory
vserver!1!rule!900!match!directory = /digest1
vserver!1!rule!900!match!final = 0
vserver!1!rule!900!auth = plain
vserver!1!rule!900!auth!methods = digest
vserver!1!rule!900!auth!realm = Test is the realm
vserver!1!rule!900!auth!passwdfile = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Digest - Plain: Authorization required"

        self.request          = "GET /digest1/file HTTP/1.0\r\n"
        self.expected_error   = 401
        self.expected_content = [ "WWW-Authenticate: Digest", "qop=", "algorithm=" ]
        self.forbiden_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "digest1")
        self.WriteFile (www, "digest1/file", 0444, MAGIC)
        passfile = self.WriteFile (www, "digest1/.passwd", 0444, "user:password\n")
        
        self.conf = CONF % (passfile)
