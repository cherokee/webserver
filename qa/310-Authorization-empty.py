import base64

from qa_conf import *
from qa_base import *

MAGIC        = "Cherokee supports old crypt password hashes"
REALM        = "realm"
USER         = "username"
PASSWD       = "alo"

CONF = """
vserver!1!rule!3100!match = directory
vserver!1!rule!3100!match!directory = /htpasswd_plain_empty
vserver!1!rule!3100!match!final = 0
vserver!1!rule!3100!auth = htpasswd
vserver!1!rule!3100!auth!methods = basic
vserver!1!rule!3100!auth!realm = %s
vserver!1!rule!3100!auth!passwdfile = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)

        self.name             = "Authorization: Basic empty"
        self.expected_error   = 401
        self.request          = "GET /htpasswd_plain_empty/file HTTP/1.0\r\n" + \
                                "Authorization: Basic \r\n"

    def Prepare (self, www):
        tdir  = self.Mkdir (www, "htpasswd_plain_empty")
        passf = self.WriteFile (tdir, "passwd", 0o444, f'{USER}:{PASSWD}\n')
        self.WriteFile (tdir, "file", 0o444, MAGIC)

        self.conf = CONF % (REALM, passf)
