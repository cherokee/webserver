from base import *

MAGIC = "The one and only.. Cherokee! :-)"
HOST  = "request_mini"

CONF = """
vserver!1160!nick = <host>
vserver!1160!document_root = %s

vserver!1160!rule!10!match = default
vserver!1160!rule!10!handler = file

vserver!1160!rule!11!match = request
vserver!1160!rule!11!match!request = ^/$
vserver!1160!rule!11!handler = redir
vserver!1160!rule!11!handler!rewrite!1!show = 0
vserver!1160!rule!11!handler!rewrite!1!regex = ^.*$
vserver!1160!rule!11!handler!rewrite!1!substring = /index.php
""" 

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Request tiny"

        self.request           = "GET / HTTP/1.1\r\n"   + \
                                 "Host: %s\r\n" %(HOST) + \
                                 "Connection: Close\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["<?php", "index.php"]

    def Prepare (self, www):
        host_dir = self.Mkdir (www, "tmp_host_request_mini")

        self.conf = CONF % (host_dir)
        self.conf = self.conf.replace ('<host>', HOST)

        for php in self.php_conf.split("\n"):
            self.conf += "vserver!1160!rule!%s\n" % (php)

        self.WriteFile (host_dir, "index.php", 0444, '<?php echo "%s" ?>' %(MAGIC))
        
    def Precondition (self):
        return os.path.exists (look_for_php())

