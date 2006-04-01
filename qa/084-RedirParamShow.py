from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Redir Param redirection"
        self.request          = "GET /redirparam2/123/cosa HTTP/1.0\r\n"
        self.expected_error   = 301
        self.expected_content = "/this_is_the_new_path?arg1=123&arg2=cosa"
        self.conf             = """Directory /redirparam2 {
                                     Handler redir {
                                        Show Rewrite "^/([0-9]+)/([^\.\?/]+)$" "/this_is_the_new_path?arg1=$1&arg2=$2"
                                     }
                                }"""
    def Prepare (self, www):
        self.Mkdir (www, "redirparam2")

