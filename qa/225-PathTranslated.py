from qa_base import *

DIR       = "PathTranslated1"
PATH_INFO = "/this/is/path_info"

CONF = f"""
vserver!1!rule!2250!match = directory
vserver!1!rule!2250!match!directory = /{DIR}
vserver!1!rule!2250!handler = cgi
"""

CGI_BASE = """#!/bin/sh
echo "Content-Type: text/plain"
echo
echo "PATH_TRANSLATED: >${PATH_TRANSLATED}<"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "CGI: PATH_TRANSLATED"

        self.request          = f"GET /{DIR}/test{PATH_INFO} HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 200

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test", 0o755, CGI_BASE)

        pt = www + PATH_INFO
        self.expected_content = f"PATH_TRANSLATED: >{pt}<"
