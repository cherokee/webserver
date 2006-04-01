from base import *

MAGIC="The_index_page_should_contain_this"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Directory index, dirlist"

        self.request          = "GET /index1/ HTTP/1.0\r\n"
        self.conf             = "Directory /index1 { Handler dirlist }"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "index1")
        self.Mkdir (www, "index1/dir1")
        self.Mkdir (www, "index1/"+MAGIC)
        self.Mkdir (www, "index1/dir2")

