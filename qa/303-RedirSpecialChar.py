from base import *

DIR  = "redir303"

CONF = ("""
vserver!1!rule!3030!match = directory
vserver!1!rule!3030!match!directory = /%s
vserver!1!rule!3030!handler = redir
vserver!1!rule!3030!handler!rewrite!1!show = 1
vserver!1!rule!3030!handler!rewrite!1!substring = /hello world?out=there
""") % (DIR)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Redir 303 special chars"
        self.request           = "GET /%s/something HTTP/1.0\r\n" % (DIR)
        self.expected_error    = 301
	self.forbidden_content = "hello world"
        self.conf              = CONF

