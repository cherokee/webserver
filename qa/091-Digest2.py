from qa_conf import *
from qa_base import *

MAGIC  = "Show this"
REALM  = "realm"
USER   = "username"
PASSWD = "itissecret"

CONF = """
vserver!1!rule!910!match = directory
vserver!1!rule!910!match!directory = /digest2
vserver!1!rule!910!match!final = 0
vserver!1!rule!910!auth = plain
vserver!1!rule!910!auth!methods = digest
vserver!1!rule!910!auth!realm = %s
vserver!1!rule!910!auth!passwdfile = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Digest - Plain: Valid user/passwd pair"

        self.expected_error   = 200
        self.expected_content = MAGIC

    def JustBefore (self, www):
        # It will read a validad nonce value just before each test
        #
        nested = TestBase(__file__)
        nested.request = "GET /digest2/file HTTP/1.0\r\n"
        nested.Run(HOST, PORT)

        # Parse the authentication information line
        #
        nested.digest = self.Digest()
        vals = nested.digest.ParseHeader (nested.reply)

        # Calculate the response value
        #
        response = nested.digest.CalculateResponse (USER, REALM, PASSWD, "GET", "/digest2/file",
                                                    vals["nonce"], vals["qop"], vals["cnonce"], vals["nc"])

        # At this point, we got a valid nonce, so let's write the
        # request..
        self.request = "GET /digest2/file HTTP/1.0\r\n" +\
                       "Authorization: Digest response=\"{}\", username=\"{}\", realm=\"{}\", uri=\"{}\", nonce=\"{}\", qop=\"{}\", algorithm=\"{}\"\r\n".format(response, USER, REALM, "/digest2/file", vals["nonce"], vals["qop"], vals["algorithm"])


    def Prepare (self, www):
        # Create the infrastructure
        #
        self.Mkdir (www, "digest2")
        self.WriteFile (www, "digest2/file", 0o444, MAGIC)
        passfile = self.WriteFile (www, "digest2/.passwd", 0o444, f"{USER}:{PASSWD}\n")

        # Set the configuration
        #
        self.conf = CONF % (REALM, passfile)
