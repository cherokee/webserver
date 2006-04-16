from base import *

DOMAIN = "server_domain_to_dir2"
URL    = "http://www.example.com/dir/subdir"
DIR    = "/dir1"
PATH   = "/file1/param"

CONF = """        
vserver!%s!document_root = %s
vserver!%s!directory!%s!handler = redir
vserver!%s!directory!%s!handler!rewrite!1!show = 1
vserver!%s!directory!%s!handler!rewrite!1!regex = ^(.*)$
vserver!%s!directory!%s!handler!rewrite!1!substring = %s$1
vserver!%s!directory!%s!priority = 10
"""

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

        self.conf = CONF % (DOMAIN, srvr, DOMAIN, DIR, DOMAIN, DIR,
                            DOMAIN, DIR, DOMAIN, DIR, URL, DOMAIN, DIR)

