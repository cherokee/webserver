from base import *

MAGIC = "Virtual Host test magic string"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Virtual Host"

        self.request          = "GET /file HTTP/1.1\r\n" +\
                                "Host: cherokee.test\r\n"

        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        dir = self.Mkdir (www, "vhost1")
        self.WriteFile (www, "vhost1/file", 0444, MAGIC)
        
        self.conf = """Server cherokee.test {
                         DocumentRoot %s
                         Directory / { Handler common }
                    }""" % (dir)

