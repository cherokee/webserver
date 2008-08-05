from base import *

HEADER_CONTENT = """
============================================
= This is the header file of the directory =
============================================
"""

CONF = """
vserver!1!rule!1140!match = directory
vserver!1!rule!1140!match!directory = /header_file2
vserver!1!rule!1140!handler = dirlist
vserver!1!rule!1140!handler!notice_files = noexits1.txt,header.txt,noexits2.txt
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "dirlist w/ multiple HeaderFile"

        self.request           = "GET /header_file2/ HTTP/1.0\r\n"
        self.expected_error    = 200        
        self.expected_content  = [HEADER_CONTENT, "file1", "file2", "file3"]
        self.forbidden_content = ["header.txt"]
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, "header_file2")

        self.WriteFile (d, "file1", 0444, "")
        self.WriteFile (d, "file2", 0444, "")
        self.WriteFile (d, "file3", 0444, "")

        self.WriteFile (d, "header.txt", 0444, HEADER_CONTENT)
