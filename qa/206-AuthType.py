from base import *
from base64 import encodestring

DIR    = 'auth_type_1'
LOGIN  = "this_is_the_login"
PASSWD = "password_word"

CONF = """
vserver!1!rule!2060!match = directory
vserver!1!rule!2060!match!directory = /%s
vserver!1!rule!2060!handler = cgi
vserver!1!rule!2060!auth = authlist
vserver!1!rule!2060!auth!methods = basic
vserver!1!rule!2060!auth!realm = Test
vserver!1!rule!2060!auth!list!1!user = %s
vserver!1!rule!2060!auth!list!1!password = %s
""" % (DIR, LOGIN, PASSWD)

CGI_BASE = """#!/bin/sh
echo "Content-Type: text/plain"
echo
# Forbidden
echo "AUTH_TYPE is $AUTH_TYPE"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "AuthType: CGI"
        self.request           = "GET /%s/file HTTP/1.0\r\n" % (DIR) + \
                                 "Authorization: Basic %s\r\n" % (
                                    encodestring ("%s:%s"%(LOGIN,PASSWD))[:-1])
        self.expected_error    = 200
        self.conf              = CONF
        self.expected_content  = ['AUTH_TYPE is Basic']
        self.forbidden_content = ['Forbidden']

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "file", 0755, CGI_BASE)

