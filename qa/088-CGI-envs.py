from base import *

ENV1_NAME  = "WHATEVER"
ENV2_NAME  = "SECOND"

ENV1_VALUE = "Value1"
ENV2_VALUE = "This is the second one"

CONF = """
vserver!1!rule!880!match = directory
vserver!1!rule!880!match!directory = /cgienvs
vserver!1!rule!880!handler = cgi
vserver!1!rule!880!handler!env!%s = %s
vserver!1!rule!880!handler!env!%s = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "CGI env variables"

        self.request           = "GET /cgienvs/cgi.cgi HTTP/1.0\r\n"
        self.conf              = CONF % (ENV1_NAME, ENV1_VALUE, ENV2_NAME, ENV2_VALUE)
        self.expected_error    = 200
        self.expected_content  = ["Env1 %s = %s" % (ENV1_NAME, ENV1_VALUE), 
						    "Env2 %s = %s" % (ENV2_NAME, ENV2_VALUE)]

    def Prepare (self, www):
        self.Mkdir (www, "cgienvs")
        self.WriteFile (www, "cgienvs/cgi.cgi", 0555,
                        """#!/bin/sh

                        echo "Content-Type: text/plain"
                        echo
                        echo "Env1 %s = $%s"
                        echo "Env2 %s = $%s"
                        """ % (ENV1_NAME, ENV1_NAME, ENV2_NAME, ENV2_NAME))
