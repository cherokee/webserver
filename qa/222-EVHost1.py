from base import *

MAGIC  = '<a href="http://www.alobbs.com">Alvaro</a> tests QA #222.'
DOMAIN = '222-qa.a-test-this-is.com'

CONF = """
vserver!2220!nick = test0222
vserver!2220!match = rehost
vserver!2220!match!regex!1 = ^222-qa
vserver!2220!document_root = %s
vserver!2220!evhost = evhost
vserver!2220!evhost!tpl_document_root = %s/${domain}/web
vserver!2220!rule!1!match = default
vserver!2220!rule!1!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "EVHost: ${domain}"
        self.request          = "GET /file HTTP/1.1\r\n" +\
                                "Connection: Close\r\n" + \
                                "Host: %s\r\n" %(DOMAIN)
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d1 = self.Mkdir (www, "test_222_general")
        ev = self.Mkdir (www, "test_222_evhost")
        d2 = self.Mkdir (ev,  "%s/%s/web"%(ev,DOMAIN))

        self.WriteFile (d2, "file", 0444, MAGIC)
        self.conf = CONF % (d1, ev)
