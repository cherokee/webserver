from base import *

CONF = """
vserver!0440!nick = missing.host1
vserver!0440!document_root = /faked

vserver!0440!user_dir = public_html
vserver!0440!domain!1 = missing.host1

vserver!0440!user_dir!rule!1!match = default
vserver!0440!user_dir!rule!1!handler = common
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Invalid home"

        self.request          = "GET /~missing/file HTTP/1.1\r\n" + \
                                "Connection: Close\r\n" + \
                                "Host: missing.host1\r\n"
        self.conf             = CONF
        self.expected_error   = 404

