from conf import *
from base import *

MAGIC  = "Show this"
REALM  = "realm"
USER   = "username"
PASSWD = ""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Digest - Plain: empty passwd"
 
        self.expected_error   = 200
        self.expected_content = MAGIC
        
    def JustBefore (self, www):
        # It will read a validad nonce value just before each test
        #
        nested = TestBase()
        nested.request = "GET /digest_empty/file HTTP/1.0\r\n"
        nested.Run(PORT, 0)

        # Parse the authentication information line
        #
        nested.digest = self.Digest()
        vals = nested.digest.ParseHeader (nested.reply)

        # Calculate the response value
        #
        response = nested.digest.CalculateResponse (USER, REALM, PASSWD, "GET", "/digest_empty/file", 
                                                    vals["nonce"], vals["qop"], vals["cnonce"], vals["nc"])

        # At this point, we got a valid nonce, so let's write the 
        # request..
        self.request = "GET /digest_empty/file HTTP/1.0\r\n" +\
                       "Authorization: Digest response=\"%s\", username=\"%s\", realm=\"%s\", uri=\"%s\", nonce=\"%s\", qop=\"%s\", algorithm=\"%s\"\r\n" % \
                       (response, USER, REALM, "/digest_empty/file", vals["nonce"], vals["qop"], vals["algorithm"])


    def Prepare (self, www):
        # Create the infrastructure
        #
        self.Mkdir (www, "digest_empty")
        self.WriteFile (www, "digest_empty/file", 0444, MAGIC)
        passfile = self.WriteFile (www, "digest_empty/.passwd", 0444, "%s:%s\n" % (USER, PASSWD))

        # Set the configuration
        #
        self.conf             = """
           Directory /digest_empty {
               Handler file
               Auth Digest {
                  Name "%s"
                  Method plain {
                    PasswdFile %s
                  }
               }
           }""" % (REALM, passfile)
