from base import *

MAGIC = "This shouldn't be sent"

CONF = """
vserver!1!rule!500!match = directory
vserver!1!rule!500!match!directory = /head-body
vserver!1!rule!500!handler = common
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Head doesn't include body"

        self.request           = "HEAD /head-body/test HTTP/1.0\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.forbidden_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "head-body")
        self.WriteFile (www, "head-body/test", 0444, MAGIC)

