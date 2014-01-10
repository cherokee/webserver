from base import *

DIR       = "dirlist_transferencoding1"

CONF = """
vserver!1!rule!3040!match = directory
vserver!1!rule!3040!match!directory = /<dir>
vserver!1!rule!3040!handler = dirlist
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Dirlist: Transfer Encoding"

        self.request           = "GET /%s/ HTTP/1.1\r\n" % (DIR)  + \
                                 "Host: localhost\r\n"            + \
                                 "Connection: Keep-Alive\r\n\r\n" + \
                                 "OPTIONS / HTTP/1.0\r\n"

        self.expected_content  = ["Transfer-Encoding: chunked"]
        self.expected_error    = 200

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.conf = CONF.replace('<dir>', DIR)
