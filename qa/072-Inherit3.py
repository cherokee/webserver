from base import *

COMMENT = "/* This is a PHP comment */"
MAGIC   = "This is th magic string"

CONF = """
vserver!1!rule!720!match = directory
vserver!1!rule!720!match!directory = /inherit3/dir1/dir2/dir3
vserver!1!rule!720!handler = phpcgi
vserver!1!rule!720!handler!interpreter = %s

vserver!1!rule!721!match = directory
vserver!1!rule!721!match!directory = /inherit3
vserver!1!rule!721!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name              = "Inherit config: deeper fist"
        self.request           = "GET /inherit3/dir1/dir2/dir3/deeper.php HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = COMMENT
        self.conf              = CONF % (look_for_php())

    def Prepare (self, www):
        self.Mkdir (www, "inherit3/dir1/dir2/dir3")
        self.WriteFile (www, "inherit3/dir1/dir2/dir3/deeper.php", 0444,
                        '<?php %s echo "%s"; ?>'%(COMMENT,MAGIC))

    def Precondition (self):
        return os.path.exists (look_for_php())

