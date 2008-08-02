import os
import string
from base import *

PUBLIC_HTML = "public_html"

CONF = """
vserver!0450!nick = testhost1
vserver!0450!document_root = /faked

vserver!0450!user_dir = %s
vserver!0450!domain!1 = testhost1

vserver!0450!user_dir!rule!1!match = default
vserver!0450!user_dir!rule!1!handler = common
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Valid home"

        self.conf             = CONF % (PUBLIC_HTML)
        self.expected_error   = 200

    def Precondition (self):
        # Get the current username
        try:
            self.user = os.getlogin()
        except OSError:
            # Fixme: Why is happening this?
            return False

        self.request          = "GET /~%s/ HTTP/1.0\r\n" % (self.user)

        # Read the /etc/passwd file
        f = open ("/etc/passwd", "r")
        pwuser = filter(lambda x: x.find(self.user) == 0, f.readlines())
        f.close()

        # Sanity check
        if len(pwuser) <= 1:
            return False

        # Get the home directory
        home = string.split (pwuser[0], ":")[5]
        public_html = os.path.join (home, PUBLIC_HTML)

        # Look for the public_html directory
        if not os.path.exists(public_html):
            return False

        return True

