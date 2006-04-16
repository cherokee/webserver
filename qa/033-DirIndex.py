from base import *

MAGIC = "The_index_page_should_contain_this"

CONF = """
vserver!default!directory!/index1!handler = dirlist
vserver!default!directory!/index1!priority = 330
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Directory index, dirlist"

        self.request          = "GET /index1/ HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "index1")
        self.Mkdir (www, "index1/dir1")
        self.Mkdir (www, "index1/"+MAGIC)
        self.Mkdir (www, "index1/dir2")

