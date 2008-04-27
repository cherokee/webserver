import os
import base64
from base import *

USER="cherokeeqa"
PASS="cherokeeqa"

CONF = """
vserver!default!rule!610!match = directory
vserver!default!rule!610!match!directory = /privpam
vserver!default!rule!610!match!final = 0
vserver!default!rule!610!auth = pam
vserver!default!rule!610!auth!methods = basic
vserver!default!rule!610!auth!realm = Test PAM
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Auth PAM"

        # Build the authentication string
        auth = base64.encodestring("%s:%s" % (USER,PASS))
        if auth[-1] == "\n": auth = auth[:-1]

        self.request          = "GET /privpam/ HTTP/1.0\r\n" + \
                                "Authorization: Basic %s\r\n" % (auth)
        self.expected_error   = 200

    def Prepare (self, www):
        if not self.Precondition():
            return

        directory = self.Mkdir (www, "privpam")
        self.conf = CONF

    def Precondition (self):
        # It will only work it the server runs as root
        if os.getuid() != 0:
            return False

        try:
            # Check that pam module was compiled
            pams = filter(lambda x: "pam" in x, os.listdir(CHEROKEE_MODS))
            if len(pams) < 1:
                return False

            # Read the /etc/passwd file
            f = open ("/etc/passwd", "r")
            pwuser = filter(lambda x: x.find(USER) == 0, f.readlines())
            f.close
        except:
            return False

        # Sanity check
        if len(pwuser) is not 1:
            return False

        return True
