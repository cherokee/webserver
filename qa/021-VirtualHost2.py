from base import *

MAGIC = "Second virtual Host test magic string"

CONF = """
vserver!first.domain!document_root = %s
vserver!first.domain!domain!1 = first.domain
vserver!first.domain!domain!2 = second.domain
vserver!first.domain!directory!/!handler = common
vserver!first.domain!directory!/!priority = 10
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Virtual Host II"
        self.request          = "GET /file HTTP/1.1\r\n" +\
                                "Host: second.domain\r\n"

        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, "vhost2")
        self.WriteFile (d, "file", 0444, MAGIC)

        self.conf = CONF % (d)

