from base import *

MAGIC = "I_bet_Cherokee_is_better"
URL   = "http://www.example.com"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Request Redir compact"
        self.request           = "GET /req_redir_compact1/%s/ HTTP/1.0\r\n" % (MAGIC) 

        self.conf              = """Request "^/req_redir_compact1/(.*)/" {
                                      Handler redir {
                                         Show Rewrite "%s/$1"
                                      }
                                 }""" % (URL)

        self.expected_error    = 301
        self.expected_content  = "%s/%s" % (URL, MAGIC)
