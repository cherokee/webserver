from base import *
from util import *

DIR = "174RespinLimit"

CONF = """
vserver!1!rule!1740!match = request
vserver!1!rule!1740!match!request = ^/%s/(.*)$
vserver!1!rule!1740!handler = redir
vserver!1!rule!1740!handler!rewrite!1!show = 0
vserver!1!rule!1740!handler!rewrite!1!substring = /%s/$1/x
""" % (DIR, DIR)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name           = "Respin Limit"
        self.request        = "GET /%s/a HTTP/1.0\r\n" % (DIR)
        self.expected_error = 500
        self.conf           = CONF
