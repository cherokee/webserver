from base import *

DIR       = "error_contentlength1"

CONF = """
vserver!1!rule!3050!match = directory
vserver!1!rule!3050!match!directory = /<dir>
vserver!1!rule!3050!handler = custom_error
vserver!1!rule!3050!handler!error = 400
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Error: Content-Length"

        self.request           = "GET /%s/ HTTP/1.0\r\n" % (DIR)
        self.expected_content  = ["Content-Length: "]
        self.expected_error    = 400

    def Prepare (self, www):
        self.conf = CONF.replace('<dir>', DIR)
