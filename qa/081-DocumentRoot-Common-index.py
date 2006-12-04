from base import *

MAGIC1 = "First part"
MAGIC2 = "Second part"

CONF = """
vserver!default!directory!/dr_common_index!handler = common
vserver!default!directory!/dr_common_index!document_root = %s
vserver!default!directory!/dr_common_index!priority = 810
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name              = "DocumentRoot: common, index file"
        self.request           = "GET /dr_common_index/test_index.php HTTP/1.0\r\n" 
        self.expected_error    = 200
        self.expected_content  = MAGIC1 + MAGIC2
        self.forbidden_content = ["<?php", "Index of"]

    def Prepare (self, www):
        d = self.Mkdir (www, "dr_common_index/in/si/de")
        self.WriteFile (d, "test_index.php", 0444,
                        '<?php echo "%s"."%s"; ?>' % (MAGIC1, MAGIC2))

        self.conf = CONF % (d)

    def Precondition (self):
        return os.path.exists (look_for_php())


