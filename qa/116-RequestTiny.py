from base import *

MAGIC = "The one and only.. Cherokee! :-)"
HOST  = "request_mini"

CONF = """
vserver!<domain>!document_root = %s
vserver!<domain>!domain!1 = <domain>

vserver!<domain>!rule!10!match = default
vserver!<domain>!rule!10!handler = file

vserver!<domain>!rule!11!match = request
vserver!<domain>!rule!11!match!request = ^/$
vserver!<domain>!rule!11!handler = redir
vserver!<domain>!rule!11!handler!rewrite!1!show = 0
vserver!<domain>!rule!11!handler!rewrite!1!regex = ^.*$
vserver!<domain>!rule!11!handler!rewrite!1!substring = /index.php
""" 

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Request tiny"

        self.request           = "GET / HTTP/1.1\r\n" +\
                                 "Host: %s\r\n" %(HOST)
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["<?php", "index.php"]

    def Prepare (self, www):
        host_dir = self.Mkdir (www, "tmp_host_request_mini")

        self.conf = CONF % (host_dir)
        self.conf = self.conf.replace ('<domain>', HOST)

        for php in self.php_conf.split("\n"):
            self.conf += "vserver!%s!rule!%s\n" % (HOST, php)

        self.WriteFile (host_dir, "index.php", 0444, '<?php echo "%s" ?>' %(MAGIC))
        
    def Precondition (self):
        return os.path.exists (look_for_php())

