import os
from qa_base import *

DIR1      = "131_post_zero_php_len1"
DIR2      = "131_post_zero_php_len2"
MAGIC     = 'alvaro=alobbs.com'
FORBIDDEN = "Cherokee: The Open Source web server"

CONF  = f"""
vserver!1!rule!1310!match = directory
vserver!1!rule!1310!match!directory = /{DIR1}
vserver!1!rule!1310!handler = redir
vserver!1!rule!1310!handler!rewrite!1!show = 0
vserver!1!rule!1310!handler!rewrite!1!substring = /{DIR2}/test.php
vserver!1!rule!1310!match!final = 1
"""

PHP_BASE = f"""
<?php
  /* {FORBIDDEN} */
  echo "POST alvaro=" . $_POST['alvaro'];
?>"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "PHP: Post with zero length (Redir)"

        self.request           = f"POST /{DIR1}/ HTTP/1.0\r\n" +\
                                 "Content-type: application/x-www-form-urlencoded\r\n" +\
                                 f"Content-length: {len(MAGIC)}\r\n"
        self.post              = MAGIC
        self.conf              = CONF
        self.expected_error    = 200
        self.expected_content  = f"POST {MAGIC}"

    def Prepare (self, www):
        d = self.Mkdir (www, DIR2)
        self.WriteFile (d, "test.php", 0o755, PHP_BASE)

    def Precondition (self):
        return os.path.exists (look_for_php())
