from base import *

# Tests: http://bugs.cherokee-project.com/1207
#
# When using a complex rule match, the child match objects were
# accessing their own (empty) configuration objects instead of its
# parents where the real configuration was found.

DIR_WEB   = "web287"
DIR_LOCAL = "DirAnd-parent_properties"
FILE      = "filename_to_be_found"

CONF = """
vserver!1!rule!2870!match = and
vserver!1!rule!2870!match!left = directory
vserver!1!rule!2870!match!left!directory = /%s
vserver!1!rule!2870!match!right = request
vserver!1!rule!2870!match!right!request = .*
vserver!1!rule!2870!handler = dirlist
vserver!1!rule!2870!document_root = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Complex rules: props inheritance"

        self.request           = "GET /%s/ HTTP/1.0\r\n" % (DIR_WEB)
        self.expected_error    = 200
        self.expected_content  = FILE

    def Prepare (self, www):
        d = self.Mkdir (www, DIR_LOCAL)
        self.WriteFile (d, FILE)

        self.conf = CONF %(DIR_WEB, d)
