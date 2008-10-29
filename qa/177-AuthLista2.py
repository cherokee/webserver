from base import *
from base64 import encodestring

DIR     = 'authlist2'
LOGIN1  = "fake"
PASSWD1 = "barfoo"
LOGIN2  = "harry_potter"
PASSWD2 = "the-trick"

CONF = """
vserver!1!rule!1770!match = directory
vserver!1!rule!1770!match!directory = /%s
vserver!1!rule!1770!match!final = 0
vserver!1!rule!1770!auth = authlist
vserver!1!rule!1770!auth!methods = basic
vserver!1!rule!1770!auth!realm = Test
vserver!1!rule!1770!auth!list!1!user = %s
vserver!1!rule!1770!auth!list!1!password = %s
vserver!1!rule!1770!auth!list!2!user = %s
vserver!1!rule!1770!auth!list!2!password = %s
""" % (DIR, LOGIN1, PASSWD1, LOGIN2, PASSWD2)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "AuthList basic: match 2"
        self.request          = "GET /%s/ HTTP/1.0\r\n" % (DIR) + \
                                "Authorization: Basic %s\r\n" % (
                                    encodestring ("%s:%s"%(LOGIN1,PASSWD1))[:-1])
        self.expected_error   = 200
        self.conf             = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
