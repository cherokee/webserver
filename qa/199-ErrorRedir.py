from base import *

DOMAIN       = "test199"
DOESNT_EXIST = "This_does_not_exist"
CGI_FILE     = "handle_404.cgi"
FORBIDDEN    = "This text shouldn't appear"


CONF = """
vserver!199!nick = %s
vserver!199!document_root = %s

vserver!199!rule!1!match = default
vserver!199!rule!1!handler = file

vserver!199!rule!2!match = extensions
vserver!199!rule!2!match!extensions = cgi
vserver!199!rule!2!handler = cgi
vserver!199!rule!2!handler!error_handler = 1

vserver!199!error_handler = error_redir
vserver!199!error_handler!404!show = 0
vserver!199!error_handler!404!url = /%s
"""

CGI_BASE = """#!/bin/sh
echo "Status: 404"
echo "Content-Type: text/plain"
echo
# %s
echo "This" "is" "the cgi output."
echo "REDIRECT_URL: $REDIRECT_URL"
""" % (FORBIDDEN)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Internal redir on error"

        self.request           = "GET /%s HTTP/1.0\r\n" % (DOESNT_EXIST) + \
                                 "Host: %s\r\n" % (DOMAIN)
        self.expected_error    = 404
        self.forbidden_content = FORBIDDEN
        self.expected_content  = "REDIRECT_URL: /%s" % (DOESNT_EXIST)

    def Prepare (self, www):
        d = self.Mkdir (www, DOMAIN)
        self.WriteFile (d, CGI_FILE, 0755, CGI_BASE)

        self.conf = CONF % (DOMAIN, d, CGI_FILE)
