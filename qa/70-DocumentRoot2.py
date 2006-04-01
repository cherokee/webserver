from base import *

MAGIC="This is the magic string for the DocumentRoot test - file"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "DocumentRoot, file"
        self.request          = "GET /droot2/file HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        self.Mkdir     (www, "documentroot2/subdir/subdir")
        self.WriteFile (www, "documentroot2/subdir/subdir/file", 0444, MAGIC)

        self.conf = """Directory /droot2 {
                          Handler file
                          DocumentRoot %s/documentroot2/subdir/subdir
                    }""" % (www)
