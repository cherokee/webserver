from base import *
from base64 import encodestring

DIR    = 'authlist1'
LOGIN  = "harry_potter"
PASSWD = "the-trick"

CONF = """
vserver!1!rule!1760!match = directory
vserver!1!rule!1760!match!directory = /%s
vserver!1!rule!1760!match!final = 0
vserver!1!rule!1760!auth = authlist
vserver!1!rule!1760!auth!methods = basic
vserver!1!rule!1760!auth!realm = Test
vserver!1!rule!1760!auth!list!1!user = %s
vserver!1!rule!1760!auth!list!1!password = %s
""" % (DIR, LOGIN, PASSWD)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "AuthList basic: match"
        self.request          = "GET /%s/ HTTP/1.0\r\n" % (DIR) + \
                                "Authorization: Basic %s\r\n" % (
                                    encodestring ("%s:%s"%(LOGIN,PASSWD))[:-1])
        self.expected_error   = 200
        self.conf             = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
