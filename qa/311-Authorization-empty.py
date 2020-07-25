import base64

from base import *

MAGIC  = "Don't show this"

CONF = """
vserver!1!rule!3110!match = directory
vserver!1!rule!3110!match!directory = /digest_empty_1
vserver!1!rule!3110!match!final = 0
vserver!1!rule!3110!auth = plain
vserver!1!rule!3110!auth!methods = digest
vserver!1!rule!3110!auth!realm = Test is the realm
vserver!1!rule!3110!auth!passwdfile = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Authorization: Digest empty"

        self.request          = "GET /digest_empty_1/file HTTP/1.0\r\n" + \
                                "Authorization: Digest \r\n"
        self.expected_error   = 401
        self.expected_content = [ "WWW-Authenticate: Digest", "qop=", "algorithm=" ]
        self.forbiden_content = MAGIC

    def Prepare (self, www):
        tdir = self.Mkdir (www, "digest_empty_1")
        self.WriteFile (tdir, "file", 0444, MAGIC)
        passfile = self.WriteFile (tdir, ".passwd", 0444, "user:password\n")

        self.conf = CONF % (passfile)
