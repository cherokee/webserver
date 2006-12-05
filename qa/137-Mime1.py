from base import *

TYPE       = "example/ejemplo"
EXTENSION  = "mime_test_1"
MIME_TYPES = """
application/java-archive                        jar
application/java-serialized-object              ser
application/java-vm                             class
%s                                              %s
x-world/x-vrml                                  vrm vrml wrl
""" % (TYPE, EXTENSION)

CONF = """
server!mime_files = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Mime type I"

        self.request          = "GET /mime1/file.%s HTTP/1.0\r\n" % (EXTENSION)
        self.expected_error   = 200
        self.expected_content = "Content-Type: %s" % (TYPE)

    def Prepare (self, www):
        d = self.Mkdir (www, "mime1")
        m = self.WriteFile (d, "mime.types", 0666, MIME_TYPES)
        f = self.WriteFile (d, "file.%s" % (EXTENSION))

        self.conf = CONF % (m)
