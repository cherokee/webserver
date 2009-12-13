from base import *

DOMAIN  = "218_drupal_clean_urls"
REQUEST = "/users/1/edit"

CONF = """
vserver!2180!nick = %s
vserver!2180!document_root = %s

vserver!2180!rule!20!match = extensions
vserver!2180!rule!20!match!extensions = cgi
vserver!2180!rule!20!handler = cgi

vserver!2180!rule!10!match = default
vserver!2180!rule!10!handler = redir
vserver!2180!rule!10!handler!rewrite!1!show = 0
vserver!2180!rule!10!handler!rewrite!1!regex = ^(.*)$
vserver!2180!rule!10!handler!rewrite!1!substring = /index.cgi?q=$1
"""

CGI_BASE = """#!/bin/sh
echo "Content-type: text/html"
echo ""
echo "REQUEST_URI: ($REQUEST_URI)"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Drupal: clean URLs"

        self.request           = "GET %s HTTP/1.1\r\n" %(REQUEST) +\
                                 "Host: %s\r\n" %(DOMAIN)         +\
                                 "Connection: Close\r\n"

        self.expected_error    = 200
        self.expected_content  = "REQUEST_URI: (%s)" % (REQUEST)

    def Prepare (self, www):
        srvr = self.Mkdir (www, "domain_%s" % (DOMAIN))
        self.conf = CONF % (DOMAIN, srvr)

        self.WriteFile (srvr, "index.cgi", 0755, CGI_BASE)
