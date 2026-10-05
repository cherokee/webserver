from qa_base import *
from qa_util import *

DIR = "174RespinLimit"

CONF = f"""
vserver!1!rule!1740!match = request
vserver!1!rule!1740!match!request = ^/{DIR}/(.*)$
vserver!1!rule!1740!handler = redir
vserver!1!rule!1740!handler!rewrite!1!show = 0
vserver!1!rule!1740!handler!rewrite!1!substring = /{DIR}/$1/x
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name           = "Respin Limit"
        self.request        = f"GET /{DIR}/a HTTP/1.0\r\n"
        self.expected_error = 500
        self.conf           = CONF
