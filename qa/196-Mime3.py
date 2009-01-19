from base import *

DIR       = "mime3"
TYPE      = "example/ejemplo3"
EXTENSION = "thing.mime_test_3"

MIME_TYPES = """
mime!%s!extensions = %s
""" % (TYPE, EXTENSION)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Mime type III: dot in extension"

        self.request          = "GET /%s/file.%s HTTP/1.0\r\n" % (DIR, EXTENSION)
        self.expected_error   = 200
        self.expected_content = "Content-Type: %s" % (TYPE)
        self.conf             = MIME_TYPES

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, "file.%s" % (EXTENSION))

