import base64

from conf import *
from base import *

MAGIC         = "Cherokee supports Apache MD5 implementation"
REALM         = "realm"
USER          = "username"
PASSWD        = "alo"
PASSWD_APACHE = "$apr1$VVusx/..$B6P.9/IK81S3M1QNVdfdX0"


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)

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

        self.conf             = """Directory /apachemd5_1 {
                                     Handler file
                                     Auth Basic {
                                          Name "%s"
                                          Method htpasswd { PasswdFile %s }
                                     }
                                }""" % (REALM, passf)
