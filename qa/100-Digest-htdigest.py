from conf import *
from base import *

MAGIC  = "Cherokee supports htdigest files"
REALM  = "realm"
USER   = "username"
PASSWD = "itissecret"
DIR    = "digest_htdigest1"

CONF = """
vserver!default!directory!/%s!auth = htdigest
vserver!default!directory!/%s!auth!methods = digest
vserver!default!directory!/%s!auth!realm = %s
vserver!default!directory!/%s!auth!passwdfile = %s
vserver!default!directory!/%s!priority = 1000
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Digest - htdigest: Valid user/passwd"
 
        self.expected_error   = 200
        self.expected_content = MAGIC
        
    def JustBefore (self, www):
        # It will read a validad nonce value just before each test
        #
        nested = TestBase()
        nested.request = "GET /%s/file HTTP/1.0\r\n" % (DIR)
        nested.Run(PORT, 0)

        # Parse the authentication information line
        #
        nested.digest = self.Digest()
        vals = nested.digest.ParseHeader (nested.reply)

        # Calculate the response value
        #
        response = nested.digest.CalculateResponse (USER, REALM, PASSWD, "GET", "/%s/file" % (DIR), 
                                                    vals["nonce"], vals["qop"], vals["cnonce"], vals["nc"])

        # At this point, we got a valid nonce, so let's write the 
        # request..
        self.request = "GET /%s/file HTTP/1.0\r\n" % (DIR) +\
                       "Authorization: Digest response=\"%s\", username=\"%s\", realm=\"%s\", uri=\"%s\", nonce=\"%s\", qop=\"%s\", algorithm=\"%s\"\r\n" % \
                       (response, USER, REALM, "/%s/file" %(DIR), vals["nonce"], vals["qop"], vals["algorithm"])


    def Prepare (self, www):
        from md5 import md5

        # Create the infrastructure
        #
        test_dir = self.Mkdir (www, DIR)
        self.WriteFile (test_dir, "file", 0444, MAGIC)

        # Prepare the password file
        #
        md5obj = md5()
        md5obj.update("%s:%s:%s" % (USER, REALM, PASSWD))
        password = md5obj.hexdigest()

        passfile = self.WriteFile (test_dir, ".passwd", 0444, "%s:%s:%s\n" % (USER, REALM, password))

        # Set the configuration
        #
        self.conf = CONF % (DIR, DIR, DIR, REALM, DIR, passfile, DIR)

        self.conf2             = """
           Directory /%s {
               Handler file
               Auth Digest {
                  Name "%s"
                  Method htdigest {
                    PasswdFile %s
                  }
               }
           }""" % (DIR, REALM, passfile)
