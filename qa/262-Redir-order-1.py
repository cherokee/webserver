from base import *

DIR = "QA262"

CONF = """
vserver!1!rule!2620!match = directory
vserver!1!rule!2620!match!directory = /%(DIR)s
vserver!1!rule!2620!handler = redir
vserver!1!rule!2620!handler!rewrite!1!show = 1
vserver!1!rule!2620!handler!rewrite!1!regex = ^/(.*)$
vserver!1!rule!2620!handler!rewrite!1!substring = /First_Rule/$1
vserver!1!rule!2620!handler!rewrite!2!show = 1
vserver!1!rule!2620!handler!rewrite!2!regex = ^/(.*)$
vserver!1!rule!2620!handler!rewrite!2!substring = /Second_Rule/$1
vserver!1!rule!2620!handler!rewrite!3!show = 1
vserver!1!rule!2620!handler!rewrite!3!regex = ^/(.*)$
vserver!1!rule!2620!handler!rewrite!3!substring = /Third_Rule/$1
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Request Redir evaluation order"
        self.request           = "GET /%(DIR)s/whatever HTTP/1.0\r\n" % (globals())
        self.conf              = CONF %(globals())
        self.expected_error    = 301
        self.expected_content  = "First_Rule"
        self.forbidden_content = ['Second_Rule', 'Third_Rule']

