from base import *
from base64 import encodestring

DIR     = 'authlist3'
LOGIN1  = "foo"
PASSWD1 = "bar"
LOGIN2  = "harry_potter"
PASSWD2 = "the-trick"

CONF = """
vserver!1!rule!1780!match = directory
vserver!1!rule!1780!match!directory = /%s
vserver!1!rule!1780!match!final = 0
vserver!1!rule!1780!auth = authlist
vserver!1!rule!1780!auth!methods = basic
vserver!1!rule!1780!auth!realm = Test
vserver!1!rule!1780!auth!list!1!user = %s
vserver!1!rule!1780!auth!list!1!password = %s
""" % (DIR, LOGIN1, PASSWD1)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "AuthList basic: mismatch"
        self.request          = "GET /%s/ HTTP/1.0\r\n" % (DIR) + \
                                "Authorization: Basic %s\r\n" % (
                                    encodestring ("%s:%s"%(LOGIN2,PASSWD2))[:-1])
        self.expected_error   = 401
        self.conf             = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
