from base import *

MAGIC="This is the magic string for the DocumentRoot test - common"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "DocumentRoot, common"
        self.request          = "GET /droot/file HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        self.Mkdir     (www, "documentroot/subdir/subdir")
        self.WriteFile (www, "documentroot/subdir/subdir/file", 0444, MAGIC)

        self.conf = """Directory /droot {
                          Handler common
                          DocumentRoot %s/documentroot/subdir/subdir
                    }""" % (www)

