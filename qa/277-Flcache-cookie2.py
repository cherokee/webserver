import itertools
from base import *

DIR     = "flcache-cookie2"
CONTENT = "Front-line can cache responses setting 'diregarded cookies'"

CONF = """
vserver!1!rule!2770!match = directory
vserver!1!rule!2770!match!directory = /%(DIR)s
vserver!1!rule!2770!handler = cgi
vserver!1!rule!2770!flcache = 1
vserver!1!rule!2770!flcache!cookies!do_cache!1!regex = foo
vserver!1!rule!2770!flcache!cookies!do_cache!2!regex = bogus
vserver!1!rule!2770!flcache!cookies!do_cache!3!regex = bar
""" %(globals())


CGI_CODE = """#!/bin/sh

echo "Content-Type: text/plain"
echo "Set-cookie: bogus=galletita"
echo "Cache-control: public"
echo

echo "%(CONTENT)s"
""" %(globals())


class TestEntry (TestBase):
    def __init__ (self, filename):
        TestBase.__init__ (self, __file__)
        self.request        = "GET /%s/%s HTTP/1.0\r\n" %(DIR, filename) +\
                              "Connection: close\r\n"
        self.expected_error = 200


class Test (TestCollection):
    counter = itertools.count()

    def __init__ (self):
        TestCollection.__init__ (self, __file__)

        self.name           = "Front-line cache: Diregarded cookie"
        self.conf           = CONF
        self.proxy_suitable = True
        self.delay          = 1

    def JustBefore (self, www):
        test_num = Test.counter.next()
        self.filename = "test277-id%s-test%s" %(id(self), test_num)

        # Write the new file
        self.WriteFile (self.local_dir, self.filename, 0755, CGI_CODE)

        # Create sub-request objects
        self.Empty()

        obj = self.Add (TestEntry (self.filename))
        obj.expected_content = ['X-Cache: MISS', CONTENT]

        obj = self.Add (TestEntry (self.filename))
        obj.expected_content = ['X-Cache: HIT', CONTENT]

    def JustAfter (self, www):
        # Clean up the local file
        fp = os.path.join (self.local_dir, self.filename)
        os.unlink (fp)
        self.filename = None

    def Prepare (self, www):
        self.local_dir = self.Mkdir (www, DIR)

    def Precondition (self):
        return not self.is_ssl
