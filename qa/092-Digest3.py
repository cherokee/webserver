from conf import *
from base import *

MAGIC   = "Do not show this"
REALM   = "realm"
USER    = "username"
PASSWD1 = "itissecret"
PASSWD2 = "Itissecret"

CONF = """
vserver!1!rule!920!match = directory
vserver!1!rule!920!match!directory = /digest3
vserver!1!rule!920!match!final = 0
vserver!1!rule!920!auth = plain
vserver!1!rule!920!auth!methods = digest
vserver!1!rule!920!auth!realm = %s
vserver!1!rule!920!auth!passwdfile = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Digest - plain: Inalid user/passwd pair"

        self.expected_error    = 401
        self.forbidden_content = MAGIC

    def JustBefore (self, www):
        # It will read a validad nonce value just before each test
        #
        nested = TestBase(__file__)
        nested.request = "GET /digest3/file HTTP/1.0\r\n"
        nested.Run(HOST, PORT)

        # Parse the authentication information line
        #
        nested.digest = self.Digest()
        vals = nested.digest.ParseHeader (nested.reply)

        # Calculate the response value
        #
        response = nested.digest.CalculateResponse (USER, REALM, PASSWD2, "GET", "/digest3/file",
                                                    vals["nonce"], vals["qop"], vals["cnonce"], vals["nc"])

        # At this point, we got a valid nonce, so let's write the
        # request..
        self.request = "GET /digest3/file HTTP/1.0\r\n" +\
                       "Authorization: Digest response=\"%s\", username=\"%s\", realm=\"%s\", uri=\"%s\", nonce=\"%s\", qop=\"%s\", algorithm=\"%s\"\r\n" % \
                       (response, USER, REALM, "/digest2/file", vals["nonce"], vals["qop"], vals["algorithm"])


    def Prepare (self, www):
        # Create the infrastructure
        #
        self.Mkdir (www, "digest3")
        self.WriteFile (www, "digest3/file", 0444, MAGIC)
        passfile = self.WriteFile (www, "digest3/.passwd", 0444, "%s:%s\n" % (USER, PASSWD1))

        # Set the configuration
        #
        self.conf = CONF % (REALM, passfile)
