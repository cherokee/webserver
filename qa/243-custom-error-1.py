from base import *

DOMAIN       = "test243"
DOESNT_EXIST = "This_does_not_exist"
CONTENT_404  = "Not found. Most probably, eaten by dog."

CONF = """
vserver!243!nick = %s
vserver!243!document_root = %s

vserver!243!rule!1!match = default
vserver!243!rule!1!handler = file

vserver!243!error_handler = error_redir
vserver!243!error_handler!404!show = 0
vserver!243!error_handler!404!url = /404.txt
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Custom Error return code"

        self.request           = "GET /%s HTTP/1.0\r\n" % (DOESNT_EXIST) + \
                                 "Host: %s\r\n" % (DOMAIN)
        self.expected_error    = 404
        self.expected_content  = CONTENT_404

    def Prepare (self, www):
        d = self.Mkdir (www, DOMAIN)
        self.WriteFile (d, "404.txt", 0644, CONTENT_404)

        self.conf = CONF % (DOMAIN, d)
