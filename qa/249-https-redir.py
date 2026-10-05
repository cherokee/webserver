# -*- coding: utf-8 -*-

from qa_base import *

DIR  = "https-redir-1"
FILE = "file"

CONF = """
vserver!1!rule!2490!match = directory
vserver!1!rule!2490!match!directory = /{}
vserver!1!rule!2490!handler = redir
vserver!1!rule!2490!handler!rewrite!1!regex = /(.*)$
vserver!1!rule!2490!handler!rewrite!1!show = 1
vserver!1!rule!2490!handler!rewrite!1!substring = https://${{host}}/{}/$1
""".format(DIR, DIR)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Redirection to https: No host"

        self.request          = f"GET /{DIR}/{FILE} HTTP/1.0\r\n"
        self.expected_error   = 301
        self.expected_content = ['Location: https://', f'/{DIR}/{FILE}']
        self.conf             = CONF
