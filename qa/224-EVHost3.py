from base import *

MAGIC  = '<a href="http://www.alobbs.com">Alvaro</a> tests QA #224.'
DOMAIN = '224-qa.users.example.com'

CONF = """
vserver!2240!nick = test0224
vserver!2240!match = rehost
vserver!2240!match!regex!1 = ^224-qa
vserver!2240!document_root = %s
vserver!2240!evhost = evhost
vserver!2240!evhost!tpl_document_root = %s/${tld}/${subdomain2}
vserver!2240!rule!1!match = default
vserver!2240!rule!1!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "EVHost: ${tld}/${subdomain2}"
        self.request          = "GET /file HTTP/1.1\r\n" +\
                                "Connection: Close\r\n" + \
                                "Host: %s\r\n" %(DOMAIN)
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d1 = self.Mkdir (www, "test_224_general")
        ev = self.Mkdir (www, "test_224_evhost")
        d2 = self.Mkdir (ev,  "%s/com/224-qa"%(ev))

        self.WriteFile (d2, "file", 0444, MAGIC)
        self.conf = CONF % (d1, ev)
