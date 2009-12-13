from base import *

CONF = """
vserver!1!rule!840!match = directory
vserver!1!rule!840!match!directory = /redirparam2
vserver!1!rule!840!handler = redir
vserver!1!rule!840!handler!rewrite!1!show = 1
vserver!1!rule!840!handler!rewrite!1!regex = ^/([0-9]+)/([^\.\?/]+)$
vserver!1!rule!840!handler!rewrite!1!substring = /this_is_the_new_path?arg1=$1&arg2=$2
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Redir Param redirection"
        self.request          = "GET /redirparam2/123/cosa HTTP/1.0\r\n"
        self.expected_error   = 301
        self.expected_content = "/this_is_the_new_path?arg1=123&arg2=cosa"
        self.conf             = CONF

    def Prepare (self, www):
        self.Mkdir (www, "redirparam2")

