from base import *

MAGIC = "First virtual Host test magic string"

CONF = """
vserver!0200!nick = test020
vserver!0200!document_root = %s
vserver!0200!domain!1 = cherokee.test
vserver!0200!rule!1!match = default
vserver!0200!rule!1!handler = common
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Virtual Host"
        self.request          = "GET /file HTTP/1.1\r\n" +\
                                "Connection: Close\r\n" + \
                                "Host: cherokee.test\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, "vhost1")
        self.WriteFile (d, "file", 0444, MAGIC)
        
        self.conf = CONF % (d)
