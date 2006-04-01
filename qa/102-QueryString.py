from base import *

PARAMS = "param1=one&param2=two&param3"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "CGI: QUERY_STRING"

        self.request          = "GET /querystring/exec.cgi?%s HTTP/1.0\r\n" % (PARAMS)
        self.conf             = "Directory /querystring { Handler cgi }"
        self.expected_error   = 200
        self.expected_content = "QUERY_STRING = %s" % (PARAMS)

    def Prepare (self, www):
        d = self.Mkdir (www, "querystring")
        self.WriteFile (d, "exec.cgi", 0755,
                        """#!/bin/sh

                        echo "Content-Type: text/plain"
                        echo
                        echo "QUERY_STRING = $QUERY_STRING"
                        """) 
