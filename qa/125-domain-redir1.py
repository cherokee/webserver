from base import *

DOMAIN = "server_domain_to_domain1"
PATH   = "/file1/param"

CONF = """
vserver!1250!nick = <domain>
vserver!1250!document_root = /faked

vserver!1250!rule!10!match = default
vserver!1250!rule!10!handler = redir
vserver!1250!rule!10!handler!rewrite!1!show = 1
vserver!1250!rule!10!handler!rewrite!1!regex = ^/(.*)$
vserver!1250!rule!10!handler!rewrite!1!substring = http://www.<domain>/$1

vserver!1251!nick = www.<domain>
vserver!1251!document_root = %s
vserver!1251!match = wildcard
vserver!1251!match!domain!1 = www.<domain>
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Domain to Domain I"

        self.request           = "GET %s HTTP/1.1\r\n" %(PATH) +\
                                 "Host: %s\r\n" %(DOMAIN)      +\
                                 "Connection: Close\r\n"

        self.expected_error    = 301
        self.expected_content  = "http://www.%s%s" % (DOMAIN, PATH)

    def Prepare (self, www):
        srvr = self.Mkdir (www, "domain_%s" % (DOMAIN))

        self.conf = CONF % (srvr)
        self.conf = self.conf.replace ('<domain>', DOMAIN)
