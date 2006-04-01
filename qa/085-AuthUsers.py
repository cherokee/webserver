from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Auth basic, with users"
        self.request          = "GET /auth1users/ HTTP/1.0\r\n"
        self.expected_error   = 401

    def Prepare (self, www):
        dir = self.Mkdir (www, "auth1users")
        self.WriteFile (www, "auth1users/passwd", 0444, 'user:cherokee\n' + 'foo:bar\n' + 'sending:sos\n')

        self.conf             = """Directory /auth1users {
                                     Auth Basic {
                                          Name "Test with Users"
                                          Method plain { PasswdFile %s }
                                          User foo, alo
                                     }
                                }""" % (dir+"/passwd")
