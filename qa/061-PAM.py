import os
import base64
from qa_base import *

USER="cherokeeqa"
PASS="cherokeeqa"

CONF = """
vserver!1!rule!610!match = directory
vserver!1!rule!610!match!directory = /privpam
vserver!1!rule!610!match!final = 0
vserver!1!rule!610!auth = pam
vserver!1!rule!610!auth!methods = basic
vserver!1!rule!610!auth!realm = Test PAM
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Auth PAM"

        # Build the authentication string
        auth = base64.b64encode(f"{USER}:{PASS}".encode()).decode()

        self.request          = "GET /privpam/ HTTP/1.0\r\n" + \
                                f"Authorization: Basic {auth}\r\n"
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

        # Check that pam module was compiled
        if not cherokee_has_plugin("pam"):
            return False

        try:
            # Read the /etc/passwd file
            f = open ("/etc/passwd", "r")
            pwuser = [x for x in f.readlines() if x.find(USER) == 0]
            f.close
        except:
            return False

        # Sanity check
        if len(pwuser) != 1:
            return False

        return True
