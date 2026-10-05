from qa_base import *

DIR     = "inherit3/dir1/dir2/dir3"
COMMENT = "This is a PHP comment"
MAGIC   = "This is th magic string"

CONF = """
vserver!1!rule!721!match = directory
vserver!1!rule!721!match!directory = /inherit3
vserver!1!rule!721!handler = file
"""

# alo:
# This QA entry does no longer make sense

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Inherit config: deeper fist"
        self.request           = f"GET /{DIR}/deeper.php HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = COMMENT
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "deeper.php", 0o444,
                        f'<?php /* {COMMENT} */ echo "{MAGIC}"; ?>')

    def Precondition (self):
        return os.path.exists (look_for_php())
