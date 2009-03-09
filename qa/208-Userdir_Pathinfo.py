import os
import string
from base import *

DOMAIN      = "208domain"
DIR         = "208/directory"
FILE        = "file.cgi"
PUBLIC_HTML = "tmp"
PATH_INFO   = "/this/is/path/info"
MAGIC       = "alobbs.com"

CONF = """
vserver!2080!nick = %s
vserver!2080!document_root = %s
vserver!2080!user_dir = %s

vserver!2080!user_dir!rule!2!match!directory = /%s
vserver!2080!user_dir!rule!2!match = directory
vserver!2080!user_dir!rule!2!handler = cgi
vserver!2080!user_dir!rule!1!match = default
vserver!2080!user_dir!rule!1!handler = file
"""

CGI_BASE = """#!/bin/sh
echo "Content-type: text/html"
echo ""
echo "PATH_INFO: $PATH_INFO"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Userdir: Pathinfo"
        self.expected_error   = 200
        self.expected_content = ["PATH_INFO: %s"%(PATH_INFO)]

    def Precondition (self):
        # ~/tmp/cherokee, alo
        tmp = self.Precondition_UserHome ("tmp")
        if not tmp:
            return False

        self.public_html, self.user = tmp
        self.request = "GET /~%s/%s/%s%s HTTP/1.0\r\n" % (self.user, DIR, FILE, PATH_INFO) + \
                       "Host: %s\r\n" % (DOMAIN)
        return True

    def Prepare (self, www):
        self.Remove (self.public_html, '208')
        droot = self.Mkdir (self.public_html, DIR)

        self.WriteFile (droot, FILE, 0555, CGI_BASE)
        self.conf = CONF % (DOMAIN, droot, PUBLIC_HTML, DIR)
