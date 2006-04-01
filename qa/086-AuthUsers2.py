from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Auth basic, with users II"
        self.request          = "GET /auth2users/ HTTP/1.0\r\n" +\
                                "Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==\r\n"
        self.expected_error   = 200

    def Prepare (self, www):
        dir = self.Mkdir (www, "auth2users")
        self.WriteFile (www, "auth2users/passwd", 0444, 'user:cherokee\n' + 'foo:bar\n' + 'Aladdin:open sesame\n')

        self.conf             = """Directory /auth2users {
                                     Auth Basic {
                                          Name "Test with Users"
                                          Method plain { PasswdFile %s }
                                          User foo, Aladdin
                                     }
                                }""" % (dir+"/passwd")
