from base import *

DOMAIN = "server_domain_to_domain1"
PATH   = "/file1/param"

CONF = """        
vserver!%s!document_root = %s
vserver!%s!directory!/!handler = redir
vserver!%s!directory!/!handler!rewrite!1!show = 1
vserver!%s!directory!/!handler!rewrite!1!regex = ^/(.*)$
vserver!%s!directory!/!handler!rewrite!1!substring = http://www.%s/$1
vserver!%s!directory!/!priority = 10

vserver!www.%s!document_root = %s
vserver!www.%s!directory!/!handler = file
vserver!www.%s!directory!/!priority = 10
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

        self.conf = CONF % (DOMAIN, srvr, DOMAIN, DOMAIN, DOMAIN, DOMAIN, DOMAIN, DOMAIN,
                            DOMAIN, srvr, DOMAIN, DOMAIN)

