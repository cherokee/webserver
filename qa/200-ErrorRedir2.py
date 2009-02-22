from base import *

DOMAIN       = "test200"
DOESNT_EXIST = "This_does_not_exist"
CGI_FILE     = "handle_404.cgi"
CGI_PATHINFO = "this/is/pathinfo"
CGI_QS       = "foo=bar&two=2"
DIR_WEB      = "public"
DIR_LOCAL    = "actual_directory"
FORBIDDEN    = "This text shouldn't appear"


CONF = """
vserver!200!nick = %(DOMAIN)s
vserver!200!document_root = %(droot)s

vserver!200!rule!1!match = default
vserver!200!rule!1!handler = file

vserver!200!rule!2!match = extensions
vserver!200!rule!2!match!extensions = cgi
vserver!200!rule!2!handler = cgi
vserver!200!rule!2!handler!error_handler = 1
vserver!200!rule!2!document_root = %(local_dir)s

vserver!200!error_handler = error_redir
vserver!200!error_handler!404!show = 0
vserver!200!error_handler!404!url = /%(CGI_FILE)s/%(CGI_PATHINFO)s
"""

CGI_BASE = """#!/bin/sh
echo "Status: 404"
echo "Content-Type: text/plain"
echo
# %s
echo "This" "is" "the cgi output."
echo "REDIRECT_URL: $REDIRECT_URL"
echo "REDIRECT_QUERY_STRING: $REDIRECT_QUERY_STRING"
echo "PATH_INFO: $PATH_INFO"
""" % (FORBIDDEN)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Internal redir on error: QS, Pathinfo"

        self.request           = "GET /%s?%s HTTP/1.0\r\n" % (DOESNT_EXIST, CGI_QS) + \
                                 "Host: %s\r\n" % (DOMAIN)
        self.expected_error    = 404
        self.forbidden_content = FORBIDDEN
        self.expected_content  = ["REDIRECT_URL: /%s" % (DOESNT_EXIST),
                                  "REDIRECT_QUERY_STRING: %s" % (CGI_QS),
                                  "PATH_INFO: /%s" % (CGI_PATHINFO)]

    def Prepare (self, www):
        droot = self.Mkdir (www, DOMAIN)
        local_dir = self.Mkdir (droot, DIR_LOCAL)
        self.WriteFile (local_dir, CGI_FILE, 0755, CGI_BASE)

        vars = globals()
        vars['droot']     = droot
        vars['local_dir'] = local_dir

        self.conf = CONF % (vars)
