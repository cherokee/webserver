from base import *

DIR     = "flcache-basic1"
FILE    = "file.txt"
CONTENT = "Front-line cache boosts Cherokee performance"

CONF = """
vserver!1!rule!2750!match = directory
vserver!1!rule!2750!match!directory = /%(DIR)s
vserver!1!rule!2750!handler = file
vserver!1!rule!2750!flcache = 1
vserver!1!rule!2750!flcache!policy = all_but_forbidden
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

        self.name           = "Front-line cache: MISS/HIT static"
        self.conf           = CONF
        self.proxy_suitable = True

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0444, CONTENT)

        # First request
        obj = self.Add (TestEntry())
        obj.expected_content = ['X-Cache: MISS', CONTENT]

        # Second request
        obj = self.Add (TestEntry())
        obj.expected_content = ['X-Cache: HIT', CONTENT]
