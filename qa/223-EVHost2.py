from base import *

MAGIC  = '<a href="http://www.alobbs.com">Alvaro</a> tests QA #223.'
DOMAIN = '223-qa.a-test-this-is.com'

CONF = """
vserver!2230!nick = test0223
vserver!2230!match = rehost
vserver!2230!match!regex!1 = ^223-qa
vserver!2230!document_root = %s
vserver!2230!evhost = evhost
vserver!2230!evhost!tpl_document_root = /whatever
vserver!2230!rule!1!match = default
vserver!2230!rule!1!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "EVHost: no ${domain}"
        self.request          = "GET /file HTTP/1.1\r\n" +\
                                "Connection: Close\r\n" + \
                                "Host: %s\r\n" %(DOMAIN)
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d1 = self.Mkdir (www, "test_223_general")
        self.WriteFile (d1, "file", 0444, MAGIC)
        self.conf = CONF % (d1)
