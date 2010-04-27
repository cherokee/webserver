from base import *

DIR       = "dirlist_symlinks1"
LINK_NAME = "symbolic_link_name"
FILE_NAME = "real_file_we_will_link"

CONF = """
vserver!1!rule!1570!match = directory
vserver!1!rule!1570!match!directory = /<dir>
vserver!1!rule!1570!handler = dirlist
vserver!1!rule!1570!handler!symlinks = 0
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Dirlist: symlinks"

        self.request           = "GET /%s/ HTTP/1.0\r\n" % (DIR)
        self.expected_error    = 200
        self.forbidden_content = LINK_NAME

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, FILE_NAME, 0666, "Test file")
        s = self.SymLink (f, os.path.join(d,LINK_NAME))

        self.conf = CONF.replace('<dir>', DIR)
