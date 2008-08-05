from base import *

MAGIC = "This_is_the_magic_key"

CONF = """
vserver!1!rule!360!match = directory
vserver!1!rule!360!match!directory = /dr_common
vserver!1!rule!360!handler = common
vserver!1!rule!360!document_root = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "DocumentRoot inside common"
        self.request          = "GET /dr_common/ HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC
        
    def Prepare (self, www):
        d = self.Mkdir (www, "dr_common_another/dir")
        self.WriteFile (d, "test_index.html", 0444, MAGIC)

        self.conf = CONF % (d)

