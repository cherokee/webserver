from base import *

MAGIC  = "Don't show this"

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

        self.conf             = """
           Directory /digest1 {
               Handler file
               Auth Digest {
                  Name "This is the realm"
                  Method plain {
                    PasswdFile %s
                  }
               }
           }""" % (passfile)
