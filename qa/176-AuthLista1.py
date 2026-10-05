from qa_base import *
import base64

DIR    = 'authlist1'
LOGIN  = "harry_potter"
PASSWD = "the-trick"

CONF = f"""
vserver!1!rule!1760!match = directory
vserver!1!rule!1760!match!directory = /{DIR}
vserver!1!rule!1760!match!final = 0
vserver!1!rule!1760!auth = authlist
vserver!1!rule!1760!auth!methods = basic
vserver!1!rule!1760!auth!realm = Test
vserver!1!rule!1760!auth!list!1!user = {LOGIN}
vserver!1!rule!1760!auth!list!1!password = {PASSWD}
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "AuthList basic: match"
        self.request          = f"GET /{DIR}/ HTTP/1.0\r\n" + \
                                "Authorization: Basic {}\r\n".format(base64.b64encode (("{}:{}".format(LOGIN,PASSWD)).encode()).decode())
        self.expected_error   = 200
        self.conf             = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
