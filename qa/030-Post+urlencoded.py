import os
from base import *

CONF = """
vserver!1!rule!300!match = directory
vserver!1!rule!300!match!directory = /post2
vserver!1!rule!300!handler = cgi
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Post with length zero"

        self.conf           = CONF
        self.request        = "POST /post2/test HTTP/1.0\r\n" +\
                              "Content-type: application/x-www-form-urlencoded\r\n" +\
                              "Content-length: 0\r\n"
        self.post           = ""
        self.expected_error = 200

    def Prepare (self, www):
        self.Mkdir (www, "post2")
        self.WriteFile (www, "post2/test", 0755,
                        """#!/bin/sh

                        echo "Content-Type: text/plain"
                        echo
                        echo ok
                        """)

