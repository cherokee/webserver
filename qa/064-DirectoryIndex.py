from base import *

MAGIC = "This is the content of the HTML index file"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Directory indexer, HTML"

        self.request          = "GET /directoryindex1/ HTTP/1.0\r\n"
        self.conf             = "Directory /directoryindex1 { Handler common }"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "directoryindex1")
        self.WriteFile (www, "directoryindex1/test_index.html", 0444, MAGIC)

    def Precondition (self):
        return os.path.exists (PHPCGI_PATH)
