from qa_base import *
import base64

DIR    = 'auth_type_1'
LOGIN  = "this_is_the_login"
PASSWD = "password_word"

CONF = f"""
vserver!1!rule!2060!match = directory
vserver!1!rule!2060!match!directory = /{DIR}
vserver!1!rule!2060!handler = cgi
vserver!1!rule!2060!auth = authlist
vserver!1!rule!2060!auth!methods = basic
vserver!1!rule!2060!auth!realm = Test
vserver!1!rule!2060!auth!list!1!user = {LOGIN}
vserver!1!rule!2060!auth!list!1!password = {PASSWD}
"""

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
        self.request           = f"GET /{DIR}/file HTTP/1.0\r\n" + \
                                 "Authorization: Basic {}\r\n".format(base64.b64encode (("{}:{}".format(LOGIN,PASSWD)).encode()).decode())
        self.expected_error    = 200
        self.conf              = CONF
        self.expected_content  = ['AUTH_TYPE is Basic']
        self.forbidden_content = ['Forbidden']

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "file", 0o755, CGI_BASE)
