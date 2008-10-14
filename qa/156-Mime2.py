from base import *

TYPE       = "example/ejemplo2"
EXTENSION  = "mime_test_2"
MAXAGE     = 1234

MIME_TYPES = """
mime!%s!extensions = %s
mime!%s!max-age = %s
""" % (TYPE, EXTENSION, TYPE, MAXAGE)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Mime type II: max-age"

        self.request          = "GET /mime2/file.%s HTTP/1.0\r\n" % (EXTENSION)
        self.expected_error   = 200
        self.expected_content = ["Content-Type: %s" % (TYPE),
                                 "Cache-Control: max-age=%s" % (MAXAGE)]

    def Prepare (self, www):
        d = self.Mkdir (www, "mime2")
        f = self.WriteFile (d, "file.%s" % (EXTENSION))

        self.conf = MIME_TYPES
