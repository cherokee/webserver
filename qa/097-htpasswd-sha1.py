import base64

from conf import *
from base import *

MAGIC       = "Cherokee supports SHA1 password hashes"
REALM       = "realm"
USER        = "username"
PASSWD      = "alo"
PASSWD_SHA1 = "{SHA}yQ4y0eYX/0yw69R4ne1+0QmBpec="

CONF = """
vserver!1!rule!970!match = directory
vserver!1!rule!970!match!directory = /htpasswd_sha1
vserver!1!rule!970!match!final = 0
vserver!1!rule!970!auth = htpasswd
vserver!1!rule!970!auth!methods = basic
vserver!1!rule!970!auth!realm = %s
vserver!1!rule!970!auth!passwdfile = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)

        auth = base64.encodestring ("%s:%s" % (USER, PASSWD))[:-1]

        self.name             = "Basic Auth, htpasswd: SHA1"
        self.expected_error   = 200
        self.expected_content = MAGIC
        self.request          = "GET /htpasswd_sha1/file HTTP/1.0\r\n" + \
                                "Authorization: Basic %s\r\n" % (auth)

    def Prepare (self, www):
        tdir  = self.Mkdir (www, "htpasswd_sha1")
        passf = self.WriteFile (tdir, "passwd", 0444, '%s:%s\n' %(USER, PASSWD_SHA1))
        self.WriteFile (tdir, "file", 0444, MAGIC)

        self.conf = CONF % (REALM, passf)
