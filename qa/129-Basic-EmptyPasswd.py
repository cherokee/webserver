from base import *
from base64 import encodestring

LOGIN="Aladdin"
PASSWD="open sesame"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Auth basic, emtpy passwd"
        self.request          = "GET /auth_empty/ HTTP/1.0\r\n" + \
                                "Authorization: Basic %s\r\n" % (encodestring ("%s:%s"%(LOGIN,""))[:-1])
        self.expected_error   = 401

    def Prepare (self, www):
        dir = self.Mkdir (www, "auth_empty")
        self.WriteFile (www, "auth_empty/passwd", 0444, '%s:%s\n' %(LOGIN,PASSWD))

        self.conf             = """Directory /auth_empty {
                                     Auth Basic {
                                          Name "Test Empty"
                                          Method plain { PasswdFile %s }
                                     }
                                }""" % (dir+"/passwd")
