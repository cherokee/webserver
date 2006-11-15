from base import *

POST_LENGTH = (50*1024)
random_str  = letters_random (POST_LENGTH)

HEADER_CONTENT = "This is the header file of the directory"

CONF = """
vserver!default!directory!/header_file1!handler = common
vserver!default!directory!/header_file1!handler!notice_files = header
vserver!default!directory!/header_file1!priority = 1130
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "dirlist w/ HeaderFile"

        self.request           = "GET /header_file1/file/ HTTP/1.0\r\n"
        self.expected_error    = 200        
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, "header_file1")
        f = self.WriteFile (d, "file", 0444, random_str)
        f = self.WriteFile (d, "header", 0444, HEADER_CONTENT)

        tmpfile = self.WriteTemp (random_str)

        self.expected_content  = "file:%s" % (tmpfile)
        self.forbidden_content = "file:%s" % (f)
