import os
import pwd
from base import *

USER = "nobody"
UID  = pwd.getpwnam(USER)[2]

CONF = """
vserver!1!rule!1380!match = directory
vserver!1!rule!1380!match!directory = /change_user2
vserver!1!rule!1380!handler = cgi
vserver!1!rule!1380!handler!change_user = 1
"""

CGI_CODE = """#!/bin/sh
echo "Content-Type: text/plain"
echo
echo "I'm `whoami`"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "ChangeUser: cgi"
        self.request          = "GET /change_user2/test HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = "I'm %s" % (USER)
        self.conf             = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, "change_user2", 0777)
        f = self.WriteFile (d, "test", 0555, CGI_CODE)
        if os.geteuid() == 0:
            os.chown (f, UID, os.getgid())

    def Precondition (self):
        # It will only work it the server runs as root
        if os.geteuid() != 0:
            return False

        return True
