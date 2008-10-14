import base64

from conf import *
from base import *

MAGIC         = "Cherokee supports Apache MD5 implementation"
REALM         = "realm"
USER          = "username"
PASSWD        = "alo"
PASSWD_APACHE = "$apr1$VVusx/..$B6P.9/IK81S3M1QNVdfdX0"

CONF = """
vserver!1!rule!950!match = directory
vserver!1!rule!950!match!directory = /apachemd5_1
vserver!1!rule!950!match!final = 0
vserver!1!rule!950!auth = htpasswd
vserver!1!rule!950!auth!methods = basic
vserver!1!rule!950!auth!realm = %s
vserver!1!rule!950!auth!passwdfile = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)

        auth = base64.encodestring ("%s:%s" % (USER, PASSWD))[:-1]

        self.name             = "Basic Auth, htpasswd: Apache MD5"
        self.expected_error   = 200
        self.expected_content = MAGIC
        self.request          = "GET /apachemd5_1/file HTTP/1.0\r\n" + \
                                "Authorization: Basic %s\r\n" % (auth)

    def Prepare (self, www):
        tdir  = self.Mkdir (www, "apachemd5_1")
        passf = self.WriteFile (tdir, "passwd", 0444, '%s:%s\n' %(USER, PASSWD_APACHE))
        self.WriteFile (tdir, "file", 0444, MAGIC)
        self.conf = CONF % (REALM, passf)
