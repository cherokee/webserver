import os
import time
from base import *

DOCUMENTATION = """
   The Hypertext Transfer Protocol (HTTP) is an application-level
   protocol for distributed, collaborative, hypermedia information
   systems. It is a generic, stateless, protocol which can be used for
   many tasks beyond its use for hypertext, such as name servers and
   distributed object management systems, through extension of its
   request methods, error codes and headers [47]. A feature of HTTP is
   the typing and negotiation of data representation, allowing systems
   to be built independently of the data being transferred.
"""

CONF = """
vserver!1!rule!3130!match = directory
vserver!1!rule!3130!match!directory = /if_range4
vserver!1!rule!3130!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "If-Range header, invalid If-Range: time"

        self.conf              = CONF
        self.expected_error    = 200

        self.expected_content  = DOCUMENTATION

    def Prepare (self, www):
        d = self.Mkdir (www, "if_range4")
        f = self.WriteFile (d, "file", 0444, DOCUMENTATION)

        self.request          = "GET /if_range4/file HTTP/1.1\r\n"  + \
                                "Host: localhost\r\n"               + \
                                "Connection: Close\r\n"             + \
                                "If-Range: Sun, 20 Feb 2000 19:00:00 GMT\r\n"            + \
                                "Range: bytes=%d-\r\n" % (len(DOCUMENTATION)-1)
