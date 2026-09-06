from qa_base import *

DIR       = "mime3"
TYPE      = "example/ejemplo3"
EXTENSION = "thing.mime_test_3"

MIME_TYPES = f"""
mime!{TYPE}!extensions = {EXTENSION}
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Mime type III: dot in extension"

        self.request          = f"GET /{DIR}/file.{EXTENSION} HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = f"Content-Type: {TYPE}"
        self.conf             = MIME_TYPES

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, f"file.{EXTENSION}")
