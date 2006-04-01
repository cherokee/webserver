from base import *

DOMAIN = "server_domain_to_dir"
URL    = "http://www.example.com/dir/subdir/"
PATH   = "dir1/file1/param"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Domain to subdir"

        self.request           = "GET /%s HTTP/1.1\r\n" %(PATH) +\
                                 "Host: %s\r\n" %(DOMAIN)
        self.expected_error    = 301
        self.expected_content  = URL+PATH

    def Prepare (self, www):
        srvr = self.Mkdir (www, "domain_%s" % (DOMAIN))

        self.conf              = """Server %s {
                                       DocumentRoot %s
                                       Directory / {
                                          Handler redir {
                                             Show Rewrite "^/(.*)$" "%s$1"
                                          }
                                       }
                                  }
                                  """ % (DOMAIN, srvr, URL)

