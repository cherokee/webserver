from base import *

MAGIC  = "This is the file number %s."
MAGIC2 = "Magic dirname"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Pipelining, file and list"

        self.request          = "GET /pipe2/file1 HTTP/1.1\r\n"   +\
                                "Host: localhost\r\n"             +\
                                "Connection: Keep-alive\r\n" +\
                                "\r\n"                            +\
                                "GET /pipe2/ HTTP/1.1\r\n"        +\
                                "Host: localhost\r\n"             +\
                                "Connection: Close\r\n"

        self.expected_error   = 200
        self.expected_content = [ MAGIC %("one"),
                                  MAGIC2 ]

    def Prepare (self, www):
        self.Mkdir (www, "pipe2")
        self.Mkdir (www, "pipe2/"+MAGIC2)
        self.WriteFile (www, "pipe2/file1", 0444, MAGIC %("one"))
        self.WriteFile (www, "pipe2/file2", 0444, MAGIC %("two"))


