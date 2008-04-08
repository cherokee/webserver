from base import *

DOMAIN = "server_domain_to_domain1"
PATH   = "/file1/param"

CONF = """        
vserver!<domain>!document_root = /faked

vserver!<domain>!domain!1 = <domain>
vserver!<domain>!rule!default!handler = redir
vserver!<domain>!rule!default!handler!rewrite!1!show = 1
vserver!<domain>!rule!default!handler!rewrite!1!regex = ^/(.*)$
vserver!<domain>!rule!default!handler!rewrite!1!substring = http://www.<domain>/$1
vserver!<domain>!rule!default!priority = 10

vserver!www.<domain>!document_root = %s
vserver!www.<domain>!domain!1 = www.<domain>
vserver!www.<domain>!rule!default!handler = file
vserver!www.<domain>!rule!default!priority = 10
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Domain to Domain I"

        self.request           = "GET %s HTTP/1.1\r\n" %(PATH) +\
                                 "Host: %s\r\n" %(DOMAIN)
        self.expected_error    = 301
        self.expected_content  = "http://www.%s%s" % (DOMAIN, PATH)

    def Prepare (self, www):
        srvr = self.Mkdir (www, "domain_%s" % (DOMAIN))

        self.conf = CONF % (srvr)
        self.conf = self.conf.replace('<domain>', DOMAIN)
