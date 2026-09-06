from qa_base import *

DIR       = "php_checkfile"
HOST: str = "test251"

MAGIC     = 'This test checks the PHP check local file'
FORBIDDEN = 'This is a comment'

CONF_PART = """
vserver!251!nick = %(HOST)s
vserver!251!document_root = %(vserver_droot)s

vserver!251!rule!%(php_plus1)s!match = directory
vserver!251!rule!%(php_plus1)s!match!directory = /%(DIR)s
vserver!251!rule!%(php_plus1)s!match!final = 0
vserver!251!rule!%(php_plus1)s!document_root = %(document_root)s

# PHP comes here

vserver!251!rule!1!match = default
vserver!251!rule!1!handler = custom_error
vserver!251!rule!1!handler!error = 403

"""

PHP_SRC = f"""<?php
   /* {FORBIDDEN} */
  phpinfo();
  echo '{MAGIC}';
?>"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "PHP: Document root + Check local file"
        self.request           = f"GET /{DIR}/internal/test.php HTTP/1.0\r\n" +\
                                 f"Host: {HOST}\r\n"
        self.expected_error    = 200
        self.forbidden_content = FORBIDDEN

    def Prepare (self, www):
        # Build directories
        vserver_droot = self.Mkdir (www, "test251_droot")
        document_root = self.Mkdir (www, "test251_outside")
        internal_dir  = self.Mkdir (www, "test251_outside/internal")

        self.WriteFile (internal_dir, "test.php", 0o666, PHP_SRC)

        # Config
        php_plus1 = int((self.php_conf or "").split('!')[0]) + 1
        self.conf = CONF_PART %(dict (locals(), **globals()))

        # Config: PHP
        for php in (self.php_conf or "").split("\n"):
            self.conf += f"vserver!251!rule!{php}\n"

        self.conf += "vserver!251!rule!10000!match!check_local_file = 1\n"

    def Precondition (self):
        return os.path.exists (look_for_php())
