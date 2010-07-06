# -*- coding: utf-8 -*-

from base import *

DIR  = "https-redir-2"
FILE = "file"

CONF = """
vserver!1!rule!2500!match = directory
vserver!1!rule!2500!match!directory = /%s
vserver!1!rule!2500!handler = redir
vserver!1!rule!2500!handler!rewrite!1!regex = /(.*)$
vserver!1!rule!2500!handler!rewrite!1!show = 1
vserver!1!rule!2500!handler!rewrite!1!substring = https://${host}/%s/$1
""" % (DIR, DIR)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Redirection to https: Host"

        self.request          = "GET /%s/%s HTTP/1.0\r\n" %(DIR, FILE) +\
                                "Host: %s\r\n" %(HOST)
        self.expected_error   = 301
        self.expected_content = ['Location: https://%s/%s/%s'%(HOST, DIR, FILE)]
        self.conf             = CONF

