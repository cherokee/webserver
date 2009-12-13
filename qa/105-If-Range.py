import os
import time
from base import *

DOCUMENTATION = """
14.27 If-Range

   If a client has a partial copy of an entity in its cache, and wishes
   to have an up-to-date copy of the entire entity in its cache, it
   could use the Range request-header with a conditional GET (using
   either or both of If-Unmodified-Since and If-Match.) However, if the
   condition fails because the entity has been modified, the client
   would then have to make a second request to obtain the entire current
   entity-body.

   The If-Range header allows a client to "short-circuit" the second
   request. Informally, its meaning is `if the entity is unchanged, send
   me the part(s) that I am missing; otherwise, send me the entire new
   entity'.

        If-Range = "If-Range" ":" ( entity-tag | HTTP-date )

   If the client has no entity tag for an entity, but does have a Last-
   Modified date, it MAY use that date in an If-Range header. (The
   server can distinguish between a valid HTTP-date and any form of
   entity-tag by examining no more than two characters.) The If-Range
   header SHOULD only be used together with a Range header, and MUST be
   ignored if the request does not include a Range header, or if the
   server does not support the sub-range operation.

   If the entity tag given in the If-Range header matches the current
   entity tag for the entity, then the server SHOULD provide the
   specified sub-range of the entity using a 206 (Partial content)
   response. If the entity tag does not match, then the server SHOULD
   return the entire entity using a 200 (OK) response.
"""

CONF = """
vserver!1!rule!1050!match = directory
vserver!1!rule!1050!match!directory = /if_range1
vserver!1!rule!1050!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "If-Range header, 200 error"

        self.conf             = CONF
        self.expected_error   = 200
        self.expected_content = DOCUMENTATION

    def Prepare (self, www):
        d = self.Mkdir (www, "if_range1")
        f = self.WriteFile (d, "file", 0444, DOCUMENTATION)

        st = os.stat (f)
        mode, ino, dev, nlink, uid, gid, size, atime, mtime, ctime = st
        times = time.strftime("%a, %d %b %Y %H:%M:%S GMT", time.gmtime(mtime - 1))

        self.request          = "GET /if_range1/file HTTP/1.1\r\n"  + \
                                "Host: localhost\r\n"               + \
                                "Connection: Close\r\n"             + \
                                "If-Range: %s\r\n" % (times)        + \
                                "Range: bytes=%d-\r\n" % (len(DOCUMENTATION))
