from base import *

SERVER  = "hidden_params"
REQUEST = "function"
PARAMS  = "one=001&two=002"

CONF = """
vserver!<domain>!document_root = %s
vserver!<domain>!domain!1 = <domain>

vserver!<domain>!rule!1!match = default
vserver!<domain>!rule!1!handler = server_info

vserver!<domain>!rule!1350!match = request
vserver!<domain>!rule!1350!match!request = ^/([^\?]*)$
vserver!<domain>!rule!1350!handler = redir
vserver!<domain>!rule!1350!handler!rewrite!1!show = 1
vserver!<domain>!rule!1350!handler!rewrite!1!substring = /index.php?q=$1

vserver!<domain>!rule!1351!match = request
vserver!<domain>!rule!1351!match!request = ^/([^\?]*)\?(.*)$
vserver!<domain>!rule!1351!handler = redir
vserver!<domain>!rule!1351!handler!rewrite!2!show = 1
vserver!<domain>!rule!1351!handler!rewrite!2!substring = /index.php?q=$1&$2
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Hidden redir with params"
        self.request          = "GET /%s?%s HTTP/1.1\r\n" % (REQUEST, PARAMS) + \
                                "Host: %s\r\n" % (SERVER)
        self.expected_error   = 301
        self.expected_content = "Location: /index.php?q=%s&%s" % (REQUEST, PARAMS)

    def Prepare (self, www):
        d  = self.Mkdir (www, "hidde_w_params_server")
        d2 = self.Mkdir (d, "hidde_w_params")

        self.conf = CONF % (d)
        self.conf = self.conf.replace ('<domain>', SERVER)

    def Precondition (self):
        return os.path.exists (look_for_php())

