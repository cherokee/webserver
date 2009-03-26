from base import *

FILE  = '216 foo bar.txt'
MAGIC = '<a href="http://www.alobbs.com/">Alvaro</a>'

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Space in file name"

        self.expected_error = 200
        self.request        = "GET /%s HTTP/1.0\r\n" % (FILE.replace(' ', '%20'))

    def Prepare (self, www):
        self.WriteFile (www, FILE, 0444, MAGIC)


