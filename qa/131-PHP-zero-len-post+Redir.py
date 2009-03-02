import os
from base import *

DIR1      = "131_post_zero_php_len1"
DIR2      = "131_post_zero_php_len2"
MAGIC     = 'alvaro=alobbs.com'
FORBIDDEN = "Cherokee: The Open Source web server"

CONF  = """
vserver!1!rule!1310!match = directory
vserver!1!rule!1310!match!directory = /%s
vserver!1!rule!1310!handler = redir
vserver!1!rule!1310!handler!rewrite!1!show = 0
vserver!1!rule!1310!handler!rewrite!1!substring = /%s/test.php
vserver!1!rule!1310!match!final = 1
""" % (DIR1, DIR2)

PHP_BASE = """
<?php
  /* %s */
  echo "POST alvaro=" . $_POST['alvaro'];
?>""" % (FORBIDDEN)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "PHP: Post with zero length (Redir)"

        self.request           = "POST /%s/ HTTP/1.0\r\n" % (DIR1) +\
                                 "Content-type: application/x-www-form-urlencoded\r\n" +\
                                 "Content-length: %d\r\n" % (len(MAGIC))
        self.post              = MAGIC
        self.conf              = CONF
        self.expected_error    = 200
        self.expected_content  = "POST %s" % (MAGIC)

    def Prepare (self, www):
        d = self.Mkdir (www, DIR2)
        self.WriteFile (d, "test.php", 0755, PHP_BASE)

    def Precondition (self):
        return os.path.exists (look_for_php())
