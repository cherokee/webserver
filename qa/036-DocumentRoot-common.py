from base import *

MAGIC = "This_is_the_magic_key"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "DocumentRoot inside common"
        self.request          = "GET /dr_common/ HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC
        
    def Prepare (self, www):
        dir = self.Mkdir (www, "dr_common_another/dir")
        self.WriteFile (www, "dr_common_another/dir/test_index.html", 0444, MAGIC)

        self.conf             = """
           Directory /dr_common {
             Handler common
             DocumentRoot %s
           }
           """ % (dir)
