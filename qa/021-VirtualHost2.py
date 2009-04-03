from base import *

MAGIC = "Second virtual Host test magic string"

CONF = """
vserver!0210!nick = first.domain
vserver!0210!document_root = %s
vserver!0210!match = wildcard
vserver!0210!match!domain!1 = first.domain
vserver!0210!match!domain!2 = second.domain
vserver!0210!rule!1!match = default
vserver!0210!rule!1!handler = common
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Virtual Host II"
        self.request          = "GET /file HTTP/1.1\r\n" +\
                                "Connection: Close\r\n" + \
                                "Host: second.domain\r\n"

        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, "vhost2")
        self.WriteFile (d, "file", 0444, MAGIC)

        self.conf = CONF % (d)

