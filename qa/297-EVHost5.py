from base import *

MAGIC  = '<a href="http://www.alobbs.com">Alvaro</a> tests QA #297.'
DOMAIN = '297-qa.users.example.com'

CONF = """
vserver!2970!nick = test0297
vserver!2970!match = rehost
vserver!2970!match!regex!1 = ^297-qa
vserver!2970!document_root = %s
vserver!2970!evhost = evhost
vserver!2970!evhost!tpl_document_root = %s/${subdomain1}/${subdomain2}/public
vserver!2970!rule!1!match = default
vserver!2970!rule!1!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "EVHost: ${subdomain1}/${subdomain2}"
        self.request          = "GET /file HTTP/1.1\r\n" +\
                                "Connection: Close\r\n" + \
                                "Host: %s\r\n" %(DOMAIN)
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d1 = self.Mkdir (www, "test_297_general")
        ev = self.Mkdir (www, "test_297_evhost")
        d2 = self.Mkdir (ev,  "%s/users/297-qa/public"%(ev))

        self.WriteFile (d2, "file", 0444, MAGIC)
        self.conf = CONF % (d1, ev)
