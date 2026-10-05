from qa_base import *
import base64

LOGIN="Aladdin"
PASSWD="open sesame"

CONF = """
vserver!1!rule!400!match = directory
vserver!1!rule!400!match!directory = /auth2
vserver!1!rule!400!match!final = 0
vserver!1!rule!400!auth = plain
vserver!1!rule!400!auth!methods = basic
vserver!1!rule!400!auth!realm = Test
vserver!1!rule!400!auth!passwdfile = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Auth basic II"
        self.request          = "GET /auth2/ HTTP/1.0\r\n" + \
                                "Authorization: Basic {}\r\n".format(base64.b64encode (("{}:{}".format(LOGIN,PASSWD)).encode()).decode())
        self.expected_error   = 200

    def Prepare (self, www):
        d = self.Mkdir (www, "auth2")
        self.WriteFile (d, "passwd", 0o444, f'{LOGIN}:{PASSWD}\n')

        self.conf = CONF % (d+"/passwd")
