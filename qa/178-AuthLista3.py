from qa_base import *
import base64

DIR     = 'authlist3'
LOGIN1  = "foo"
PASSWD1 = "bar"
LOGIN2  = "harry_potter"
PASSWD2 = "the-trick"

CONF = f"""
vserver!1!rule!1780!match = directory
vserver!1!rule!1780!match!directory = /{DIR}
vserver!1!rule!1780!match!final = 0
vserver!1!rule!1780!auth = authlist
vserver!1!rule!1780!auth!methods = basic
vserver!1!rule!1780!auth!realm = Test
vserver!1!rule!1780!auth!list!1!user = {LOGIN1}
vserver!1!rule!1780!auth!list!1!password = {PASSWD1}
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "AuthList basic: mismatch"
        self.request          = f"GET /{DIR}/ HTTP/1.0\r\n" + \
                                "Authorization: Basic {}\r\n".format(base64.b64encode (("{}:{}".format(LOGIN2,PASSWD2)).encode()).decode())
        self.expected_error   = 401
        self.conf             = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
