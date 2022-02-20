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
vserver!1!rule!3120!match = directory
vserver!1!rule!3120!match!directory = /if_range3
vserver!1!rule!3120!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "If-Range header with Etag, 206 error"

        self.conf              = CONF
        self.forbidden_content = DOCUMENTATION
        self.expected_error    = 206

        l = len(DOCUMENTATION)

        self.start = 4
        self.end   = 10
        self.MAGIC = DOCUMENTATION[self.start:self.end+1]
        self.expected_content  = ["Content-Length: %d" % (len(self.MAGIC)),
                                  "Content-Range: bytes %d-%d/%d" % (self.start, self.end, l),
                                  self.MAGIC]


    def JustBefore (self, www):
        # It will read the generated ETag
        #
        nested = TestBase(__file__)
        nested.request = "GET /if_range3/file HTTP/1.1\r\n" + \
                         "Host: localhost\r\n"              + \
                         "Connection: Close\r\n"

        nested.Run(HOST, PORT)

        # Parse the ETag
        pos1 = nested.reply.find ('ETag: "')
        pos2 = nested.reply.find ('"\r', pos1)
        etag = nested.reply[pos1+7:pos2]

        self.request = "GET /if_range3/file HTTP/1.1\r\n"  + \
                       "Host: localhost\r\n"               + \
                       "Connection: Close\r\n"             + \
                       "If-Range: \"%s\"\r\n" % (etag)     + \
                       "Range: bytes=%d-%d\r\n" % (self.start, self.end)

    def Prepare (self, www):
        d = self.Mkdir (www, "if_range3")
        f = self.WriteFile (d, "file", 0444, DOCUMENTATION)
