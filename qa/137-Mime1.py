from qa_base import *

TYPE       = "example/ejemplo"
EXTENSION  = "mime_test_1"

MIME_TYPES = f"""
mime!application/java-archive!extensions = jar
mime!application/java-serialized-object!extensions = ser
mime!application/java-vm!extensions = class
mime!{TYPE}!extensions = {EXTENSION}
mime!x-world/x-vrml!extensions = vrm vrml wrl
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Mime type I"

        self.request          = f"GET /mime1/file.{EXTENSION} HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = f"Content-Type: {TYPE}"

    def Prepare (self, www):
        d = self.Mkdir (www, "mime1")
        f = self.WriteFile (d, f"file.{EXTENSION}")

        self.conf = MIME_TYPES
