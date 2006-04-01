from base import *

HEADER_CONTENT = """
============================================
= This is the header file of the directory =
============================================
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "dirlist w/ multiple HeaderFile"

        self.request           = "GET /header_file2/ HTTP/1.0\r\n"
        self.expected_error    = 200        
        self.expected_content  = [HEADER_CONTENT, "file1", "file2", "file3"]
        self.forbidden_content = ["header.txt"]

        self.conf              = """Directory /header_file2 {
                                       Handler dirlist {
                                          HeaderFile noexits1.txt, header.txt, noexits2.txt
                                       }
                                  }"""

    def Prepare (self, www):
        d = self.Mkdir (www, "header_file2")

        self.WriteFile (d, "file1", 0444, "")
        self.WriteFile (d, "file2", 0444, "")
        self.WriteFile (d, "file3", 0444, "")

        self.WriteFile (d, "header.txt", 0444, HEADER_CONTENT)
