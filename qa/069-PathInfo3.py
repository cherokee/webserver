from base import *

PATH_INFO = "/param1/param2/param3"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "PathInfo, cgi"

        self.request           = "GET /pathinfo3/test%s HTTP/1.0\r\n" %(PATH_INFO)
        self.conf              = "Directory /pathinfo3 { Handler cgi }"
        self.expected_error    = 200
        self.expected_content  = "PathInfo is: "+PATH_INFO

    def Prepare (self, www):
        self.Mkdir (www, "pathinfo3")
        self.WriteFile (www, "pathinfo3/test", 0555,
                        """#!/bin/bash

                        echo "Content-type: text/html"
                        echo ""
                        echo "PathInfo is: $PATH_INFO"
                        """)

