import itertools
from qa_base import *

DIR     = "flcache-basic1"
CONTENT = "Front-line cache boosts Cherokee performance"

CONF = f"""
vserver!1!rule!2750!match = directory
vserver!1!rule!2750!match!directory = /{globals()['DIR']}
vserver!1!rule!2750!handler = file
vserver!1!rule!2750!flcache = 1
vserver!1!rule!2750!flcache!policy = all_but_forbidden
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

        self.name           = "Front-line cache: MISS/HIT static"
        self.conf           = CONF
        self.proxy_suitable = True
        self.delay          = 1

    def JustBefore (self, www):
        test_num = next(Test.counter)
        self.filename = f"test275-id{id(self)}-test{test_num}"

        # Write the new file
        self.WriteFile (self.local_dir, self.filename, 0o444, CONTENT)

        # Create sub-request objects
        self.Empty()

        obj = self.Add (TestEntry (self.filename))
        obj.expected_content = ['X-Cache: MISS', CONTENT]

        obj = self.Add (TestEntry (self.filename))
        obj.expected_content = ['X-Cache: HIT', CONTENT]

    def JustAfter (self, www):
        # Clean up the local file
        assert self.filename is not None
        fp = os.path.join (self.local_dir, self.filename)
        os.unlink (fp)
        self.filename = None

    def Prepare (self, www):
        # Create the directory
        self.local_dir = self.Mkdir (www, DIR)

    def Precondition (self):
        return not self.is_ssl
