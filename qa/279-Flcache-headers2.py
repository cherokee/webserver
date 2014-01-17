import types
from base import *

DIR     = "flcache-headers2"
FILE    = "test.cgi"
CONTENT = "Front-line tries to be a compliance proxy"

CONF = """
vserver!1!rule!2790!match = directory
vserver!1!rule!2790!match!directory = /%(DIR)s
vserver!1!rule!2790!handler = file
vserver!1!rule!2790!flcache = 1
vserver!1!rule!2790!flcache!policy = all_but_forbidden
""" %(globals())


class TestEntry (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.request        = "GET /%s/%s HTTP/1.0\r\n" %(DIR, FILE) +\
                              "Connection: close\r\n"
        self.expected_error = 200


class Test (TestCollection):
    def __init__ (self):
        TestCollection.__init__ (self, __file__)

        self.name           = "Front-line: Duplicated headers"
        self.conf           = CONF
        self.proxy_suitable = True
        self.delay          = 1

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0444, CONTENT)

        def CustomTest (self):
            header = self.reply[:self.reply.find("\r\n\r\n")+2]

            # Check for dupped headers
            for l in header.split('\n')[1:]:
                if not l:
                    continue
                h, _ = l.split(':', 1)
                if header.count (h) > 1:
                    return -1

        # First request
        obj = self.Add (TestEntry())
        obj.expected_content = [CONTENT]

        # Second request
        obj = self.Add (TestEntry())
        obj.expected_content = [CONTENT]
        obj.CustomTest = types.MethodType (CustomTest, obj)

    def Precondition (self):
        return not self.is_ssl
