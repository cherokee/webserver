from base import *

SERVER  = "hidden_params"
REQUEST = "function"
PARAMS  = "one=001&two=002"

CONF = """
vserver!1350!nick = %s
vserver!1350!document_root = %s

vserver!1350!rule!1!match = default
vserver!1350!rule!1!handler = server_info

vserver!1350!rule!10!match = request
vserver!1350!rule!10!match!request = ^/([^\?]*)$
vserver!1350!rule!10!handler = redir
vserver!1350!rule!10!handler!rewrite!1!show = 1
vserver!1350!rule!10!handler!rewrite!1!substring = /index.php?q=$1

vserver!1350!rule!20!match = request
vserver!1350!rule!20!match!request = ^/([^\?]*)\?(.*)$
vserver!1350!rule!20!handler = redir
vserver!1350!rule!20!handler!rewrite!2!show = 1
vserver!1350!rule!20!handler!rewrite!2!substring = /index.php?q=$1&$2
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Hidden redir with params"
        self.request          = "GET /%s?%s HTTP/1.1\r\n" % (REQUEST, PARAMS) + \
                                "Host: %s\r\n" % (SERVER)                     + \
                                "Connection: Close\r\n"
        self.expected_error   = 301
        self.expected_content = "Location: /index.php?q=%s&%s" % (REQUEST, PARAMS)

    def Prepare (self, www):
        d  = self.Mkdir (www, "hidde_w_params_server")
        d2 = self.Mkdir (d, "hidde_w_params")

        self.conf = CONF % (SERVER, d)

    def Precondition (self):
        return os.path.exists (look_for_php())

