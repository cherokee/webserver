import os
from qa_base import *

DIR       = "post_zero_php_len1"
MAGIC     = '<a href="http://www.alobbs.com/">Alvaro</a>'
FORBIDDEN = 'Cherokee, a kill app'

CONF  = f"""
vserver!1!rule!1940!match = directory
vserver!1!rule!1940!match!directory = /{DIR}
vserver!1!rule!1940!handler = cgi
"""

PHP_BASE = f"""
<?php
  /* {FORBIDDEN} */
  echo '{MAGIC}';
?>"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "PHP: Post with zero length"

        self.request           = f"POST /{DIR}/test.php HTTP/1.0\r\n"+\
                                 "Content-type: application/x-www-form-urlencoded\r\n" +\
                                 "Content-length: 0\r\n"
        self.post              = ""
        self.conf              = CONF
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = FORBIDDEN

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test.php", 0o755, PHP_BASE)

    def Precondition (self):
        return os.path.exists (look_for_php())
