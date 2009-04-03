from base import *

SERVER    = "server_wo_userdir"
USERNAME  = "faked"
MAGIC     = "If UserDir isn't set, it should threat ~ as a common character"
FILENAME  = "file"

CONF = """
vserver!1210!nick = %s
vserver!1210!document_root = %s

vserver!1210!match = wildcard
vserver!1210!match!domain!1 = <domain>

vserver!1210!rule!10!match = default
vserver!1210!rule!10!handler = common
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "UserDir request without UserDir"

        self.request           = "GET /~%s/%s HTTP/1.1\r\n" % (USERNAME, FILENAME) +\
                                 "Host: %s\r\n" % (SERVER)                         +\
                                 "Connection: Close\r\n"

        self.expected_error    = 200
        self.expected_content  = MAGIC

    def Prepare (self, www):
        d  = self.Mkdir (www, "%s_dir" % (SERVER))

        d2 = self.Mkdir (d, "~%s" % (USERNAME))
        self.WriteFile (d2, FILENAME, 0444, MAGIC);

        self.conf = CONF % (SERVER, d)
