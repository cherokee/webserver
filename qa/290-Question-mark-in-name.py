from qa_base import *

DIR   = "test290"
FILE  = "what?ever.txt"
MAGIC = "Files can contain question marks"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Question mark in name"

        self.request          = "GET /{}/{} HTTP/1.0\r\n".format(DIR, FILE.replace('?','%3f'))
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0o444, MAGIC)
