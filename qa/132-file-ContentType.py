from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Content-Type header"
        self.request          = "GET /content_type1/file.jpg HTTP/1.0\r\n" 
        self.expected_error   = 200
        self.excepted_content = ["Content-Type", "image/jpeg"]

    def Prepare (self, www):
        d = self.Mkdir (www, "content_type1")
        self.WriteFile (d, "file.jpg", 0444, " ")

