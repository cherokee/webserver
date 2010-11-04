from base import *

NICK    = "nick-259"
DOMAIN1 = "test-1-259"
DOMAIN2 = "test-2-259"
ERROR   = 403

CONF = """
vserver!259!nick = %(NICK)s
vserver!259!document_root = /dev/null

vserver!259!match = v_or
vserver!259!match!left = wildcard
vserver!259!match!left!domain!1 = %(DOMAIN1)s
vserver!259!match!right = wildcard
vserver!259!match!right!domain!1 = %(DOMAIN2)s

vserver!259!rule!1!match = default
vserver!259!rule!1!handler = custom_error
vserver!259!rule!1!handler!error = %(ERROR)s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name           = "VRule OR"
        self.request        = "GET / HTTP/1.0\r\n" + \
                              "Host: %s\r\n" % (DOMAIN2)
        self.expected_error = ERROR
        self.conf           = CONF %(globals())
