from base import *

DOMAIN = "server_domain_to_domain1"
PATH   = "/file1/param"

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

        self.conf              = """Server %s {
                                       DocumentRoot %s
                                       Directory / {
                                          Handler redir {
                                              Show Rewrite "^/(.*)$" "http://www.%s/$1"
                                          }
                                       }
                                  }
                                  Server www.%s {
                                       DocumentRoot %s
                                       Directory / {
                                          Handler file
                                       }
                                  }
                                  """ % (DOMAIN, srvr, DOMAIN, DOMAIN, srvr)

