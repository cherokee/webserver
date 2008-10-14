from base import *

HOST = "server_domain_to_dir"
URL  = "http://www.example.com/dir/subdir/"
PATH = "dir1/file1/param"

CONF = """
vserver!1170!nick = <host>
vserver!1170!document_root = %s

vserver!1170!rule!10!match = default
vserver!1170!rule!10!handler = redir
vserver!1170!rule!10!handler!rewrite!1!show = 1
vserver!1170!rule!10!handler!rewrite!1!regex = ^/(.*)$
vserver!1170!rule!10!handler!rewrite!1!substring = %s$1
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Domain to subdir"

        self.request           = "GET /%s HTTP/1.1\r\n" %(PATH) + \
                                 "Host: %s\r\n" %(HOST)         + \
                                 "Connection: Close\r\n"
        self.expected_error    = 301
        self.expected_content  = URL+PATH

    def Prepare (self, www):
        srvr = self.Mkdir (www, "domain_%s" % (HOST))

        self.conf = CONF % (srvr, URL)
        self.conf = self.conf.replace ('<host>', HOST)
