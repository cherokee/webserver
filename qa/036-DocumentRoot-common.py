from base import *

MAGIC = "This_is_the_magic_key"

CONF = """
vserver!default!directory!/dr_common!handler = common
vserver!default!directory!/dr_common!document_root = %s
vserver!default!directory!/dr_common!priority = 360
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

