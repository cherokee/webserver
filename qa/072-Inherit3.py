from base import *

COMMENT = "/* This is a PHP comment */"
MAGIC   = "This is th magic string"

CONF = """
vserver!default!directory!/inherit3/dir1/dir2/dir3!handler = phpcgi
vserver!default!directory!/inherit3/dir1/dir2/dir3!handler!interpreter = %s
vserver!default!directory!/inherit3/dir1/dir2/dir3!priority = 720

vserver!default!directory!/inherit3!handler = file
vserver!default!directory!/inherit3!priority = 721
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

