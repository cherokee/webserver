import os
import string
from qa_base import *

DOMAIN      = "210domain"
DIR         = "210/directory"
FILE        = "file.cgi"
PUBLIC_HTML = "tmp"
MAGIC       = "alobbs.com"

CONF = """
vserver!2100!nick = %s
vserver!2100!document_root = %s
vserver!2100!user_dir = %s

vserver!2100!user_dir!rule!2!match!directory = /%s
vserver!2100!user_dir!rule!2!match = directory
vserver!2100!user_dir!rule!2!handler = cgi
vserver!2100!user_dir!rule!1!match = default
vserver!2100!user_dir!rule!1!handler = file
"""

CGI_BASE = """#!/bin/sh
echo "Content-type: text/html"
echo ""
echo "SCRIPT_NAME: -${SCRIPT_NAME}-"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Userdir: ScriptName"
        self.expected_error   = 200

    def Precondition (self):
        # ~/tmp/cherokee, alo
        tmp = self.Precondition_UserHome ("tmp")
        if not tmp:
            return False
        self.public_html, self.user = tmp

        self.request           = f"GET /~{self.user}/{DIR}/{FILE} HTTP/1.0\r\n" + \
                                 f"Host: {DOMAIN}\r\n"
        self.forbidden_content = ["/bin/sh", "echo"]
        self.expected_content  = [f"SCRIPT_NAME: -/~{self.user}/{DIR}/{FILE}-"]

        return True

    def Prepare (self, www):
        self.Remove (self.public_html, '210')
        droot = self.Mkdir (self.public_html, DIR)

        self.WriteFile (droot, FILE, 0o555, CGI_BASE)
        self.conf = CONF % (DOMAIN, droot, PUBLIC_HTML, DIR)
