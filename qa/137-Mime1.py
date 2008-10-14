from base import *

TYPE       = "example/ejemplo"
EXTENSION  = "mime_test_1"

MIME_TYPES = """
mime!application/java-archive!extensions = jar
mime!application/java-serialized-object!extensions = ser
mime!application/java-vm!extensions = class
mime!%s!extensions = %s
mime!x-world/x-vrml!extensions = vrm vrml wrl
""" % (TYPE, EXTENSION)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Mime type I"

        self.request          = "GET /mime1/file.%s HTTP/1.0\r\n" % (EXTENSION)
        self.expected_error   = 200
        self.expected_content = "Content-Type: %s" % (TYPE)

    def Prepare (self, www):
        d = self.Mkdir (www, "mime1")
        f = self.WriteFile (d, "file.%s" % (EXTENSION))

        self.conf = MIME_TYPES
