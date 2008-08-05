from base import *

MAGIC = "This QA tests the GeoIP rule module"
DIR   = "GeoIP1_nomatch"
FILE  = "test"

CONF = """
vserver!1!rule!1670!match = directory
vserver!1!rule!1670!match!directory = /%s
vserver!1!rule!1670!handler = cgi

vserver!1!rule!1671!match = and
vserver!1!rule!1671!match!left = geoip
vserver!1!rule!1671!match!left!countries = ES,US,UK,CA
vserver!1!rule!1671!match!right = directory
vserver!1!rule!1671!match!right!directory = /%s
vserver!1!rule!1671!handler = file
"""

CGI = """#!/bin/sh

echo "Content-Type: text/plain"
echo 
echo "%s"
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "GeoIP: no match"

        self.request           = "GET /%s/%s HTTP/1.0\r\n" % (DIR, FILE)
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["/bin/sh", "echo"]

    def Precondition (self):
        # Check that pam module was compiled
        if not cherokee_has_plugin("geoip"):
            return False
        return True

    def Prepare (self, www):
        self.conf = CONF % (DIR, DIR)

        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, FILE, 0755, CGI)
