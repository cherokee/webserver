import base64

from qa_conf import *
from qa_base import *

MAGIC        = "Cherokee supports old crypt password hashes"
REALM        = "realm"
USER         = "username"
PASSWD       = "alo"
PASSWD_CRYPT = "kA7oFNEzu4SrI"

CONF = """
vserver!1!rule!980!match = directory
vserver!1!rule!980!match!directory = /htpasswd_crypt
vserver!1!rule!980!match!final = 0
vserver!1!rule!980!auth = htpasswd
vserver!1!rule!980!auth!methods = basic
vserver!1!rule!980!auth!realm = %s
vserver!1!rule!980!auth!passwdfile = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)

        auth = base64.b64encode (f"{USER}:{PASSWD}".encode()).decode()

        self.name             = "Basic Auth, htpasswd: Crypt"
        self.expected_error   = 200
        self.expected_content = MAGIC
        self.request          = "GET /htpasswd_crypt/file HTTP/1.0\r\n" + \
                                f"Authorization: Basic {auth}\r\n"

    def Prepare (self, www):
        tdir  = self.Mkdir (www, "htpasswd_crypt")
        passf = self.WriteFile (tdir, "passwd", 0o444, f'{USER}:{PASSWD_CRYPT}\n')
        self.WriteFile (tdir, "file", 0o444, MAGIC)

        self.conf = CONF % (REALM, passf)
