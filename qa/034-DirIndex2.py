from base import *

MAGIC = "The_index_page_should_contain_this"

CONF = """
vserver!1!rule!340!match = directory
vserver!1!rule!340!match!directory = /index2
vserver!1!rule!340!handler = common
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Directory index, common"

        self.request          = "GET /index2/ HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "index2")
        self.Mkdir (www, "index2/dir1")
        self.Mkdir (www, "index2/"+MAGIC)
        self.Mkdir (www, "index2/dir2")

