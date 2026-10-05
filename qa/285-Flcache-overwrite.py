import itertools
from qa_base import *

DIR     = "flcache-replace1"
CONTENT = "Front-line cache boosts Cherokee performance"

CONF = f"""
vserver!1!rule!2850!match = directory
vserver!1!rule!2850!match!directory = /{globals()['DIR']}
vserver!1!rule!2850!flcache = allow
vserver!1!rule!2850!flcache!policy = all_but_forbidden
vserver!1!rule!2850!expiration = time
vserver!1!rule!2850!expiration!caching = public
vserver!1!rule!2850!expiration!time = 300
vserver!1!rule!2850!handler = cgi
"""

CGI_CODE = f"""#!/bin/sh

echo "Content-Type: text/plain"
echo "Cache-Control: no-store, no-cache"
echo

echo "{globals()['CONTENT']}"
"""


class TestEntry (TestBase):
    def __init__ (self, filename):
        TestBase.__init__ (self, __file__)
        self.request        = f"GET /{DIR}/{filename} HTTP/1.0\r\n" +\
                              "Connection: close\r\n"
        self.expected_error = 200


class Test (TestCollection):
    counter = itertools.count()

    def __init__ (self):
        TestCollection.__init__ (self, __file__)

        self.name           = "Front-line: Overwrite Cache-Control"
        self.conf           = CONF
        self.proxy_suitable = True
        self.delay          = 1

    def JustBefore (self, www):
        test_num = next(Test.counter)
        self.filename = f"test275-id{id(self)}-test{test_num}"

        # Write the new file
        self.WriteFile (self.local_dir, self.filename, 0o755, CGI_CODE)

        # Create sub-request objects
        self.Empty()

        # First request
        obj = self.Add (TestEntry (self.filename))
        obj.expected_content  = ['X-Cache: MISS', CONTENT, "public"]
        obj.forbidden_content = ['no-store', 'no-cache']

        # Second request
        obj = self.Add (TestEntry (self.filename))
        obj.expected_content  = ['X-Cache: HIT', CONTENT, "public"]
        obj.forbidden_content = ['no-store', 'no-cache']

    def JustAfter (self, www):
        # Clean up the local file
        assert self.filename is not None
        fp = os.path.join (self.local_dir, self.filename)
        os.unlink (fp)
        self.filename = None

    def Prepare (self, www):
        self.local_dir = self.Mkdir (www, DIR)

    def Precondition (self):
        return not self.is_ssl
