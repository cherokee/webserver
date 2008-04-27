import base64

from conf import *
from base import *

MAGIC        = "Cherokee supports old crypt password hashes"
REALM        = "realm"
USER         = "username"
PASSWD       = "alo"

CONF = """
vserver!default!rule!990!match = directory
vserver!default!rule!990!match!directory = /htpasswd_plain
vserver!default!rule!990!match!final = 0
vserver!default!rule!990!auth = htpasswd
vserver!default!rule!990!auth!methods = basic
vserver!default!rule!990!auth!realm = %s
vserver!default!rule!990!auth!passwdfile = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)

        auth = base64.encodestring ("%s:%s" % (USER, PASSWD))[:-1]

        self.name             = "Basic Auth, htpasswd: Plain"
        self.expected_error   = 200
        self.expected_content = MAGIC
        self.request          = "GET /htpasswd_plain/file HTTP/1.0\r\n" + \
                                "Authorization: Basic %s\r\n" % (auth)

    def Prepare (self, www):
        tdir  = self.Mkdir (www, "htpasswd_plain")
        passf = self.WriteFile (tdir, "passwd", 0444, '%s:%s\n' %(USER, PASSWD))
        self.WriteFile (tdir, "file", 0444, MAGIC)

        self.conf = CONF % (REALM, passf)
