from qa_base import *
import base64

LOGIN="Aladdin"
PASSWD="open sesame"

CONF = """
vserver!1!rule!1290!match = directory
vserver!1!rule!1290!match!directory = /auth_empty
vserver!1!rule!1290!match!final = 0
vserver!1!rule!1290!auth = plain
vserver!1!rule!1290!auth!methods = basic
vserver!1!rule!1290!auth!realm = Test Empty
vserver!1!rule!1290!auth!passwdfile = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Auth basic, emtpy passwd"
        self.request          = "GET /auth_empty/ HTTP/1.0\r\n" + \
                                "Authorization: Basic {}\r\n".format(base64.b64encode (("{}:{}".format(LOGIN,"")).encode()).decode())
        self.expected_error   = 401

    def Prepare (self, www):
        d = self.Mkdir (www, "auth_empty")
        f = self.WriteFile (d, "passwd", 0o444, f'{LOGIN}:{PASSWD}\n')

        self.conf = CONF % (f)
