import base64

from conf import *
from base import *

MAGIC      = "Cherokee supports MD5 password hashes"
REALM      = "realm"
USER       = "username"
PASSWD     = "alo"
PASSWD_MD5 = "$1$JJ3RnzaO$zpsGlLvKvMVrUW4ZNZ7Iw1"

CONF = """
vserver!1!rule!960!match = directory
vserver!1!rule!960!match!directory = /htpasswd_md5
vserver!1!rule!960!match!final = 0
vserver!1!rule!960!auth = htpasswd
vserver!1!rule!960!auth!methods = basic
vserver!1!rule!960!auth!realm = %s
vserver!1!rule!960!auth!passwdfile = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)

        auth = base64.encodestring ("%s:%s" % (USER, PASSWD))[:-1]

        self.name             = "Basic Auth, htpasswd: MD5"
        self.expected_error   = 200
        self.expected_content = MAGIC
        self.request          = "GET /htpasswd_md5/file HTTP/1.0\r\n" + \
                                "Authorization: Basic %s\r\n" % (auth)

    def Prepare (self, www):
        tdir  = self.Mkdir (www, "htpasswd_md5")
        passf = self.WriteFile (tdir, "passwd", 0444, '%s:%s\n' %(USER, PASSWD_MD5))
        self.WriteFile (tdir, "file", 0444, MAGIC)

        self.conf = CONF % (REALM, passf)
