from base import *

MAGIC = '<H1>xss</H1>'

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Handler Error XSS after \\0"

        self.expected_error    = 400
        self.request           = "GET / \r\n" +\
                                 "Connection: Keep-alive\r\n" +\
                                 "\r\n" +\
                                 "few\x00xx%s" % (MAGIC) 
        self.forbidden_content = MAGIC
