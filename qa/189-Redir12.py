from base import *

DIR  = "redir12"

CONF = ("""
vserver!1!rule!1890!match = directory
vserver!1!rule!1890!match!directory = /%s
vserver!1!rule!1890!handler = redir
vserver!1!rule!1890!handler!rewrite!1!show = 1
vserver!1!rule!1890!handler!rewrite!1!regex = ^""" + '/(\d)'*12 + """$
vserver!1!rule!1890!handler!rewrite!1!substring = /away/$12$11$10$9$8$7$6$5$4$3$2$1/end
""") % (DIR)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Redir 12 parameters"
        self.request          = "GET /%s/1/2/3/4/5/6/7/8/9/3/4/5 HTTP/1.0\r\n" % (DIR)
        self.expected_error   = 301
        self.expected_content = "/away/543987654321/end"
        self.conf             = CONF

