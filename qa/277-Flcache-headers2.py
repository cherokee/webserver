import types
from base import *

DIR     = "flcache-headers2"
FILE    = "test.cgi"
CONTENT = "Front-line tries to be a compliance proxy"

CONF = """
vserver!1!rule!2770!match = directory
vserver!1!rule!2770!match!directory = /%(DIR)s
vserver!1!rule!2770!handler = file
vserver!1!rule!2770!flcache = 1
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

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0444, CONTENT)

        def CustomTest (self):
            header = self.reply[:self.reply.find("\r\n\r\n")]

            # Check for dupped headers
            for l in header.split('\n')[1:]:
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

