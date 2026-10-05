from qa_base import *

DIR     = "flcache-headers1"
FILE    = "test.cgi"
CONTENT = "Front-line tries to be a compliance proxy"

CONF = f"""
vserver!1!rule!2780!match = directory
vserver!1!rule!2780!match!directory = /{globals()['DIR']}
vserver!1!rule!2780!handler = cgi
vserver!1!rule!2780!flcache = allow
vserver!1!rule!2780!flcache!policy = all_but_forbidden
"""


CGI_CODE = f"""#!/bin/sh

echo "Content-Type: text/plain"
echo "Via: foo"
echo "Cache-control: public"
echo

echo "{globals()['CONTENT']}"
"""


class TestEntry (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.request        = f"GET /{DIR}/{FILE} HTTP/1.0\r\n" +\
                              "Connection: close\r\n"
        self.expected_error = 200


class Test (TestCollection):
    def __init__ (self):
        TestCollection.__init__ (self, __file__)

        self.name           = "Front-line headers: Via"
        self.conf           = CONF
        self.proxy_suitable = True
        self.delay          = 1

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0o755, CGI_CODE)

        # First request
        obj = self.Add (TestEntry())
        obj.expected_content = [CONTENT]

        # Second request
        obj = self.Add (TestEntry())
        obj.expected_content = ['Via: foo, ', 'Age: ', CONTENT]

    def Precondition (self):
        return not self.is_ssl
