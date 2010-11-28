from base import *
from os import system

DIR  = "dir_264"

CONF = """
vserver!1!rule!2640!match = directory
vserver!1!rule!2640!match!directory = /%(DIR)s
vserver!1!rule!2640!handler = redir
vserver!1!rule!2640!handler!rewrite!1!show = 1
vserver!1!rule!2640!handler!rewrite!1!regex = (.*)
vserver!1!rule!2640!handler!rewrite!1!substring = /
vserver!1!rule!2640!expiration = time
vserver!1!rule!2640!expiration!caching = public
vserver!1!rule!2640!expiration!time = 365d
"""

# Covers bug #1056
# http://bugs.cherokee-project.com/1056
#

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Pragma: No-cache - Redirections"
        self.request           = "GET /%(DIR)s/foo HTTP/1.0\r\n" % (globals())
        self.expected_error    = 301
        self.expected_content  = ["Location: /", "Cache-Control: max-age=31536000, public"]
        self.forbidden_content = ["Pragma: no-cache", "no-cache"]
        self.conf              = CONF %(globals())
