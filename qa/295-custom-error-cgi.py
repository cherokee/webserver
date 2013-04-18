from base import *

DOMAIN       = "test295"
DOESNT_EXIST = "This_does_not_exist"
CONTENT_404  = "Not found. But it does exist now!"

CONF = """
vserver!295!nick = %s
vserver!295!document_root = %s

vserver!295!rule!1!match = default
vserver!295!rule!1!handler = cgi

vserver!295!error_handler = error_redir
vserver!295!error_handler!404!show = 0
vserver!295!error_handler!404!url = /404.sh
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Custom Error return code"

        self.request           = "GET /%s HTTP/1.0\r\n" % (DOESNT_EXIST) + \
                                 "Host: %s\r\n" % (DOMAIN)
        self.expected_error    = 200
        self.expected_content  = CONTENT_404

    def Prepare (self, www):
        d = self.Mkdir (www, DOMAIN)
        self.WriteFile (d, "404.sh", 0755, "#!/bin/sh\necho -e 'HTTP/1.0 200 OK\r\nStatus: 200 OK\r\n\r\n" + CONTENT_404 + "'")

        self.conf = CONF % (DOMAIN, d)
