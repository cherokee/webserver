from conf import *
from base import *

MAGIC  = "Cherokee supports htdigest files"
REALM  = "realm"
USER   = "username"
PASSWD = "itissecret"
DIR    = "digest_htdigest2"

ACCOUNTS=(
            ('joseph','joseph'),
            ('username','itissecret'),
         )

CONF = """
vserver!1!rule!1750!match = directory
vserver!1!rule!1750!match!directory = %s
vserver!1!rule!1750!match!final = 0
vserver!1!rule!1750!auth = htdigest
vserver!1!rule!1750!auth!methods = digest
vserver!1!rule!1750!auth!realm = %s
vserver!1!rule!1750!auth!passwdfile = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Digest - htdigest: Valid user/passwd 2"

        self.expected_error   = 200
        self.expected_content = MAGIC

    def JustBefore (self, www):
        import random
        USER, PASSWD = random.choice(ACCOUNTS)

        # It will read a validad nonce value just before each test
        #
        nested = TestBase(__file__)
        nested.request = "GET /%s/file HTTP/1.0\r\n" % (DIR)
        nested.Run(HOST, PORT)

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
        try:
            from hashlib import md5
        except ImportError:
            from md5 import md5

        # Create the infrastructure
        #
        test_dir = self.Mkdir (www, DIR)
        self.WriteFile (test_dir, "file", 0444, MAGIC)

        # Prepare the password file
        #
        kd = lambda x: md5(':'.join(x)).hexdigest()
        authlines = (':'.join( (user,REALM,kd((user,REALM,password))) ) for user,password in ACCOUNTS)


        passfile = self.WriteFile (test_dir, ".passwd", 0444, '\n'.join(authlines))

        # Set the configuration
        #
        self.conf = CONF % ('/%s'%(DIR), REALM, passfile)
