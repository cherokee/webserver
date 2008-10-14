from base import *

MAGIC = "This is the file number %s."

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Pipelining"

        self.request          = "GET /pipe1/file1 HTTP/1.1\r\n"   +\
                                "Host: localhost\r\n"             +\
                                "Connection: Keep-alive\r\n"      +\
                                "\r\n"                            +\
                                "GET /pipe1/file2 HTTP/1.1\r\n"   +\
                                "Host: localhost\r\n"             +\
                                "Connection: Keep-alive\r\n"      +\
                                "\r\n"                            +\
                                "GET /pipe1/file3 HTTP/1.1\r\n"   +\
                                "Host: localhost\r\n"             +\
                                "Connection: Close\r\n"

        self.expected_error   = 200
        self.expected_content = [ MAGIC %("one"),
                                  MAGIC %("two"),
                                  MAGIC %("three") ]

    def Prepare (self, www):
        self.Mkdir (www, "pipe1")
        self.WriteFile (www, "pipe1/file1", 0444, MAGIC %("one"))
        self.WriteFile (www, "pipe1/file2", 0444, MAGIC %("two"))
        self.WriteFile (www, "pipe1/file3", 0444, MAGIC %("three"))

