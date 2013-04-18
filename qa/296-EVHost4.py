from base import *

MAGIC  = '<a href="http://www.alobbs.com">Alvaro</a> tests QA #296.'
DOMAIN = '296-qa.users.example.com'

CONF = """
vserver!2960!nick = test0296
vserver!2960!match = rehost
vserver!2960!match!regex!1 = ^296-qa
vserver!2960!document_root = %s
vserver!2960!evhost = evhost
vserver!2960!evhost!tpl_document_root = %s/${root_domain}/${subdomain1}/public
vserver!2960!rule!1!match = default
vserver!2960!rule!1!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "EVHost: ${root_domain}/${subdomain1}"
        self.request          = "GET /file HTTP/1.1\r\n" +\
                                "Connection: Close\r\n" + \
                                "Host: %s\r\n" %(DOMAIN)
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d1 = self.Mkdir (www, "test_296_general")
        ev = self.Mkdir (www, "test_296_evhost")
        d2 = self.Mkdir (ev,  "%s/example/users/public"%(ev))

        self.WriteFile (d2, "file", 0444, MAGIC)
        self.conf = CONF % (d1, ev)
