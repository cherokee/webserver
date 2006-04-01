from base import *

DOMAIN = "server_domain_to_dir2"
URL    = "http://www.example.com/dir/subdir"
DIR    = "/dir1"
PATH   = "/file1/param"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Domain to subdir II"

        self.request           = "GET %s/%s HTTP/1.1\r\n" %(DIR, PATH) +\
                                 "Host: %s\r\n" %(DOMAIN)
        self.expected_error    = 301
        self.expected_content  = URL+PATH

    def Prepare (self, www):
        srvr = self.Mkdir (www, "domain_%s" % (DOMAIN))

        self.conf              = """Server %s {
                                       DocumentRoot %s
                                       Directory %s {
                                          Handler redir {
                                             Show Rewrite "^(.*)$" "%s$1"
                                          }
                                       }
                                  }
                                  """ % (DOMAIN, srvr, DIR, URL)

