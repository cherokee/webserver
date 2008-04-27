from base import *

DOMAIN = "server_domain_to_dir2"
URL    = "http://www.example.com/dir/subdir"
DIR    = "/dir1"
PATH   = "/file1/param"

CONF = """        
vserver!<domain>!document_root = %s
vserver!<domain>!domain!1 = <domain>

vserver!<domain>!rule!1!match = default
vserver!<domain>!rule!1!handler = server_info

vserver!<domain>!rule!10!match = directory
vserver!<domain>!rule!10!match!directory = <dir>
vserver!<domain>!rule!10!handler = redir
vserver!<domain>!rule!10!handler!rewrite!1!show = 1
vserver!<domain>!rule!10!handler!rewrite!1!regex = ^(.*)$
vserver!<domain>!rule!10!handler!rewrite!1!substring = %s$1
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

        self.conf = CONF % (srvr, URL)
        self.conf = self.conf.replace ('<domain>', DOMAIN)
        self.conf = self.conf.replace ('<dir>', DIR)

