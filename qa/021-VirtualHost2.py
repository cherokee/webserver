from base import *

MAGIC = "Virtual Host test magic string II"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Virtual Host II"
        self.request          = "GET /file HTTP/1.1\r\n" +\
                                "Host: second.domain\r\n"

        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        dir = self.Mkdir (www, "vhost2")
        self.WriteFile (www, "vhost2/file", 0444, MAGIC)
        
        self.conf = """Server first.domain, second.domain {
                         DocumentRoot %s
                         Directory / { Handler common }
                    }""" % (dir)

