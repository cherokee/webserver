from base import *

DIR   = "respin1"
MAGIC = "Cherokee_is_pure_magic"

CONF = """
vserver!1!rule!1150!match = directory
vserver!1!rule!1150!match!directory = /respin1
vserver!1!rule!1150!handler = common

vserver!1!rule!1151!match = directory
vserver!1!rule!1151!match!directory = /respin1-cgi
vserver!1!rule!1151!handler = cgi

vserver!1!rule!1152!match = request
vserver!1!rule!1152!match!request = /respin1/.+/
vserver!1!rule!1152!handler = redir
vserver!1!rule!1152!handler!rewrite!1!show = 0
vserver!1!rule!1152!handler!rewrite!1!regex = ^/respin1/(.+)/$
vserver!1!rule!1152!handler!rewrite!1!substring = /respin1-cgi/file?param=$1
"""

CGI_BASE = """#!%s
import os,cgi
print "Content-type: text/html"
print
print "param is " + cgi.parse_qs(os.getenv('QUERY_STRING'))['param'][0]
""" % (look_for_python())

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Redirection to PHP"
        self.request          = "GET /%s/%s/ HTTP/1.0\r\n" % (DIR, MAGIC)
        self.expected_error   = 200
        self.expected_content = "param is %s" % (MAGIC)
        self.conf             = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, "%s-cgi"%(DIR))
        self.WriteFile (d, "file", 0755, CGI_BASE)
