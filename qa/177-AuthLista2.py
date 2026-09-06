from qa_base import *
import base64

DIR     = 'authlist2'
LOGIN1  = "fake"
PASSWD1 = "barfoo"
LOGIN2  = "harry_potter"
PASSWD2 = "the-trick"

CONF = f"""
vserver!1!rule!1770!match = directory
vserver!1!rule!1770!match!directory = /{DIR}
vserver!1!rule!1770!match!final = 0
vserver!1!rule!1770!auth = authlist
vserver!1!rule!1770!auth!methods = basic
vserver!1!rule!1770!auth!realm = Test
vserver!1!rule!1770!auth!list!1!user = {LOGIN1}
vserver!1!rule!1770!auth!list!1!password = {PASSWD1}
vserver!1!rule!1770!auth!list!2!user = {LOGIN2}
vserver!1!rule!1770!auth!list!2!password = {PASSWD2}
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "AuthList basic: match 2"
        self.request          = f"GET /{DIR}/ HTTP/1.0\r\n" + \
                                "Authorization: Basic {}\r\n".format(base64.b64encode (("{}:{}".format(LOGIN1,PASSWD1)).encode()).decode())
        self.expected_error   = 200
        self.conf             = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
