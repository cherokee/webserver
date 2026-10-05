import base64

from qa_conf import *
from qa_base import *

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
        TestBase.__init__ (self, __file__)

        auth = base64.b64encode (f"{USER}:{PASSWD}".encode()).decode()

        self.name             = "Basic Auth, htpasswd: MD5"
        self.expected_error   = 200
        self.expected_content = MAGIC
        self.request          = "GET /htpasswd_md5/file HTTP/1.0\r\n" + \
                                f"Authorization: Basic {auth}\r\n"

    def Prepare (self, www):
        tdir  = self.Mkdir (www, "htpasswd_md5")
        passf = self.WriteFile (tdir, "passwd", 0o444, f'{USER}:{PASSWD_MD5}\n')
        self.WriteFile (tdir, "file", 0o444, MAGIC)

        self.conf = CONF % (REALM, passf)
