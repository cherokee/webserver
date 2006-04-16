from base import *
from base64 import encodestring

LOGIN="Aladdin"
PASSWD="open sesame"

CONF = """
vserver!default!directory!/auth_empty!auth = plain
vserver!default!directory!/auth_empty!auth!methods = basic
vserver!default!directory!/auth_empty!auth!realm = Test Empty
vserver!default!directory!/auth_empty!auth!passwdfile = %s
vserver!default!directory!/auth_empty!priority = 1290
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Auth basic, emtpy passwd"
        self.request          = "GET /auth_empty/ HTTP/1.0\r\n" + \
                                "Authorization: Basic %s\r\n" % (encodestring ("%s:%s"%(LOGIN,""))[:-1])
        self.expected_error   = 401

    def Prepare (self, www):
        d = self.Mkdir (www, "auth_empty")
        f = self.WriteFile (d, "passwd", 0444, '%s:%s\n' %(LOGIN,PASSWD))

        self.conf = CONF % (f)
