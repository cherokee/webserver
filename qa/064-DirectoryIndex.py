from base import *

MAGIC = "This is the content of the HTML index file"

CONF = """
vserver!1!rule!640!match = directory
vserver!1!rule!640!match!directory = /directoryindex1
vserver!1!rule!640!handler = common
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Directory indexer, HTML"

        self.request          = "GET /directoryindex1/ HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, "directoryindex1")
        self.WriteFile (d, "test_index.html", 0444, MAGIC)

    def Precondition (self):
        return os.path.exists (look_for_php())
