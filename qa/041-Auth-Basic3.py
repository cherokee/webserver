from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Auth basic III"
        self.request          = "GET /auth3/ HTTP/1.0\r\n" + \
                                "Authorization: Basic WRONG_RpbjpvcGVuIHNlc2FtZQ==\r\n"
        self.expected_error   = 401

    def Prepare (self, www):
        dir = self.Mkdir (www, "auth3")
        self.WriteFile (www, "auth3/passwd", 0444, 'Aladdin:open sesame\n')

        self.conf             = """Directory /auth3 {
                                     Handler common
                                     Auth Basic {
                                          Name "Test"
                                          Method plain { PasswdFile %s }
                                     }
                                }""" % (dir+"/passwd")
