from base import *
import os

DIR       = "php_dir_documentroot"
MAGIC     = '<a href="http://www.alobbs.com/">Alvaro</a>'
FORBIDDEN = 'This is a comment'

PHP_SCRIPT = """
<?php
   /* %s */
   echo '%s';
?>""" % (FORBIDDEN, MAGIC)

CONF = """
vserver!1!rule!1930!match = directory
vserver!1!rule!1930!match!directory = /%s
vserver!1!rule!1930!handler = common
vserver!1!rule!1930!document_root = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "PHP: DocumentRoot directory"
        self.request           = "GET /%s/index.php HTTP/1.0\r\n" % (DIR)
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = [FORBIDDEN, "<?php"]

    def Prepare (self, www):
        # Temp dir outside of the path document root
        d = tempfile.mkdtemp ("che193")
        os.chmod(d, 0755)
        self.WriteFile (d, "index.php", 0444, PHP_SCRIPT)
        self.conf = CONF % (DIR, d)

    def Precondition (self):
        return os.path.exists (look_for_php())
