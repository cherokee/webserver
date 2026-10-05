import itertools
from qa_base import *

DIR     = "flcache-cookie1"
CONTENT = "Front-line does not cache responses setting cookies"

CONF = f"""
vserver!1!rule!2760!match = directory
vserver!1!rule!2760!match!directory = /{globals()['DIR']}
vserver!1!rule!2760!handler = cgi
vserver!1!rule!2760!flcache = 1
"""


CGI_CODE = f"""#!/bin/sh

echo "Content-Type: text/plain"
echo "Set-cookie: foo=bar"
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

        self.name           = "Front-line cache: Cookie"
        self.conf           = CONF
        self.proxy_suitable = True
        self.delay          = 1

    def Prepare (self, www):
        self.local_dir = self.Mkdir (www, DIR)

    def JustBefore (self, www):
        test_num = next(Test.counter)
        self.filename = f"test276-id{id(self)}-test{test_num}"

        # Write the new file
        self.WriteFile (self.local_dir, self.filename, 0o755, CGI_CODE)

        # Create sub-request objects
        self.Empty()

        # First request
        obj = self.Add (TestEntry (self.filename))
        obj.expected_content = ['X-Cache: MISS', CONTENT]

        # Second request
        obj = self.Add (TestEntry (self.filename))
        obj.expected_content = ['X-Cache: MISS', CONTENT]

    def JustAfter (self, www):
        # Clean up the local file
        assert self.filename is not None
        fp = os.path.join (self.local_dir, self.filename)
        os.unlink (fp)
        self.filename = None

    def Precondition (self):
        return not self.is_ssl
