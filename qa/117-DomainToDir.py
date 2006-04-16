from base import *

HOST = "server_domain_to_dir"
URL  = "http://www.example.com/dir/subdir/"
PATH = "dir1/file1/param"

CONF = """
vserver!%s!document_root = %s
vserver!%s!directory!/!handler = redir
vserver!%s!directory!/!handler!rewrite!1!show = 1
vserver!%s!directory!/!handler!rewrite!1!regex = ^/(.*)$
vserver!%s!directory!/!handler!rewrite!1!substring = %s$1
vserver!%s!directory!/!priority = 10
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Domain to subdir"

        self.request           = "GET /%s HTTP/1.1\r\n" %(PATH) +\
                                 "Host: %s\r\n" %(HOST)
        self.expected_error    = 301
        self.expected_content  = URL+PATH

    def Prepare (self, www):
        srvr = self.Mkdir (www, "domain_%s" % (HOST))
        self.conf = CONF % (HOST, srvr, HOST, HOST, HOST, HOST, URL, HOST)

