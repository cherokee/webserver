from qa_base import *

MAGIC = '"><script>alert(1)</script>'

CONF = """
vserver!1!rule!3070!match = directory
vserver!1!rule!3070!match!directory = /server_info
vserver!1!rule!3070!handler = server_info
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Server Info, prevent XSS via URL"

        self.request           = f"GET /server_info/{MAGIC} HTTP/1.0\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.forbidden_content = MAGIC
