from base import *
from base64 import encodestring

LOGIN="Aladdin"
PASSWD="open sesame"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Auth basic II"
        self.request          = "GET /auth2/ HTTP/1.0\r\n" + \
                                "Authorization: Basic %s\r\n" % (encodestring ("%s:%s"%(LOGIN,PASSWD))[:-1])
        self.expected_error   = 200

    def Prepare (self, www):
        dir = self.Mkdir (www, "auth2")
        self.WriteFile (www, "auth2/passwd", 0444, '%s:%s\n' %(LOGIN,PASSWD))

        self.conf             = """Directory /auth2 {
                                     Auth Basic {
                                          Name "Test"
                                          Method plain { PasswdFile %s }
                                     }
                                }""" % (dir+"/passwd")
