from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Auth basic"
        self.request          = "GET /auth1/ HTTP/1.0\r\n"
        self.expected_error   = 401

    def Prepare (self, www):
        dir = self.Mkdir (www, "auth1")
        self.WriteFile (www, "auth1/passwd", 0444, 'user:cherokee')

        self.conf             = """Directory /auth1 {
                                     Auth Basic {
                                          Name "Test"
                                          Method plain { PasswdFile %s }
                                     }
                                }""" % (dir+"/passwd")
