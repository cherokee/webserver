from qa_base import *

TYPE       = "example/ejemplo2"
EXTENSION  = "mime_test_2"
MAXAGE     = 1234

MIME_TYPES = f"""
mime!{TYPE}!extensions = {EXTENSION}
mime!{TYPE}!max-age = {MAXAGE}
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Mime type II: max-age"

        self.request          = f"GET /mime2/file.{EXTENSION} HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = [f"Content-Type: {TYPE}",
                                 f"Cache-Control: max-age={MAXAGE}"]

    def Prepare (self, www):
        d = self.Mkdir (www, "mime2")
        f = self.WriteFile (d, f"file.{EXTENSION}")

        self.conf = MIME_TYPES
