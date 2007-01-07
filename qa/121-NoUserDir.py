from base import *

SERVER    = "server_wo_userdir"
USERNAME  = "faked"
MAGIC     = "If UserDir isn't set, it should threat ~ as a common character"
FILENAME  = "file"

CONF = """
vserver!<domain>!document_root = %s
vserver!<domain>!domain!1 = <domain>
vserver!<domain>!directory!/!handler = common
vserver!<domain>!directory!/!priority = 10
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "UserDir request without UserDir"

        self.request           = "GET /~%s/%s HTTP/1.1\r\n" % (USERNAME, FILENAME) +\
                                 "Host: %s\r\n" % (SERVER)

        self.expected_error    = 200
        self.expected_content  = MAGIC

    def Prepare (self, www):
        d  = self.Mkdir (www, "%s_dir" % (SERVER))

        d2 = self.Mkdir (d, "~%s" % (USERNAME))
        self.WriteFile (d2, FILENAME, 0444, MAGIC);

        self.conf = CONF % (d)
        self.conf = self.conf.replace ('<domain>', SERVER)
