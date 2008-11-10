from base import *

DIR  = "redir9"

CONF = """
vserver!1!rule!1890!match = directory
vserver!1!rule!1890!match!directory = /%s
vserver!1!rule!1890!handler = redir
vserver!1!rule!1890!handler!rewrite!1!show = 1
vserver!1!rule!1890!handler!rewrite!1!regex = ^/(\d)/(\d)/(\d)/(\d)/(\d)/(\d)/(\d)/(\d)/(\d)$
vserver!1!rule!1890!handler!rewrite!1!substring = /away/$9$8$7$6$5$4$3$2$1/end
""" % (DIR)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Redir 9 parameters"
        self.request          = "GET /%s/1/2/3/4/5/6/7/8/9 HTTP/1.0\r\n" % (DIR)
        self.expected_error   = 301
        self.expected_content = "/away/987654321/end"
        self.conf             = CONF

