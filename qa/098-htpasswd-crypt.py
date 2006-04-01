import base64

from conf import *
from base import *

MAGIC        = "Cherokee supports old crypt password hashes"
REALM        = "realm"
USER         = "username"
PASSWD       = "alo"
PASSWD_CRYPT = "kA7oFNEzu4SrI"


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)

        auth = base64.encodestring ("%s:%s" % (USER, PASSWD))[:-1]

        self.name             = "Basic Auth, htpasswd: Crypt"
        self.expected_error   = 200
        self.expected_content = MAGIC
        self.request          = "GET /htpasswd_crypt/file HTTP/1.0\r\n" + \
                                "Authorization: Basic %s\r\n" % (auth)

    def Prepare (self, www):
        tdir  = self.Mkdir (www, "htpasswd_crypt")
        passf = self.WriteFile (tdir, "passwd", 0444, '%s:%s\n' %(USER, PASSWD_CRYPT))
        self.WriteFile (tdir, "file", 0444, MAGIC)

        self.conf             = """Directory /htpasswd_crypt {
                                     Handler file
                                     Auth Basic {
                                          Name "%s"
                                          Method htpasswd { PasswdFile %s }
                                     }
                                }""" % (REALM, passf)
