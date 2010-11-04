from base import *

NICK    = "nick-260"
DIR     = "/dir-260"
DOMAIN1 = "test-1-260"
DOMAIN2 = "test-2-260"
DOMAIN  = "whatever-260"
ERROR1  = 100
ERROR2  = 201

CONF = """
vserver!260!nick = %(NICK)s
vserver!260!document_root = /dev/null

vserver!260!match = v_or
vserver!260!match!left = wildcard
vserver!260!match!left!domain!1 = %(DOMAIN1)s
vserver!260!match!right = wildcard
vserver!260!match!right!domain!1 = %(DOMAIN2)s

vserver!260!rule!1!match = default
vserver!260!rule!1!handler = custom_error
vserver!260!rule!1!handler!error = %(ERROR1)s

vserver!1!rule!2600!match = directory
vserver!1!rule!2600!match!directory = %(DIR)s
vserver!1!rule!2600!match!final = 1
vserver!1!rule!2600!handler = custom_error
vserver!1!rule!2600!handler!error = %(ERROR2)s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name           = "VRule OR: No match"
        self.request        = "GET %s/ HTTP/1.0\r\n" %(DIR) + \
                              "Host: %s\r\n" %(DOMAIN)
        self.expected_error = ERROR2
        self.conf           = CONF %(globals())
