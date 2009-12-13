from base import *

MAGIC      = "This is the content of the HTML index file"
DIR_LOCAL  = "pipermail169/sub1/sub2"
DIR_WEB    = "index_droot169"
DIR_INSIDE = "inside"

CONF = """
vserver!1!rule!1690!match = directory
vserver!1!rule!1690!match!directory = /%s
vserver!1!rule!1690!handler = common
vserver!1!rule!1690!document_root = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Directory indexer, DocumentRoot"

        self.request          = "GET /%s/%s/ HTTP/1.0\r\n" % (DIR_WEB, DIR_INSIDE)
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, DIR_LOCAL)
        self.conf = CONF % (DIR_WEB, d)

        i = self.Mkdir (d, DIR_INSIDE)
        self.WriteFile (i, "test_index.html", 0444, MAGIC)
