from base import *

DIR               = "dir_263"
CONTENT           = "Whatever"
IF_MODIFIED_SINCE = "Tue, 25 Sep 3121 32:36:66 GMT"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name    = "If-Modified-Since: Invalid date"
        self.request = "GET /%(DIR)s/index.html HTTP/1.0\r\n"         %(globals()) + \
                       "If-Modified-Since: %(IF_MODIFIED_SINCE)s\r\n" %(globals())

        self.expected_error   = 200
        self.expected_content = CONTENT

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, "index.html", 0444, CONTENT)

