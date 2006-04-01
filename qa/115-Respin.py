from base import *

MAGIC = "Cherokee_is_pure_magic"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Redirection to PHP"

        self.request           = "GET /respin1/%s/ HTTP/1.0\r\n" % (MAGIC)
        self.expected_error    = 200        
        self.expected_content  = "param is %s" % (MAGIC)
        self.conf              = """Directory /respin1 {
                                       Handler common
                                    }
                                    Directory /respin1-cgi {
                                       Handler phpcgi {
                                         Interpreter %s
                                       }
                                    }
                                    Request "/respin1/.+/" {
                                       Handler redir {
                                         Rewrite "^/respin1/(.+)/$" "/respin1-cgi/file?param=$1"
                                       }
                                    }
                                 """ % (PHPCGI_PATH)

    def Prepare (self, www):
        d = self.Mkdir (www, "respin1-cgi")

        self.WriteFile (d, "file", 0444,
                        '<?php echo "param is ". $_GET["param"]; ?>')

    def Precondition (self):
        return os.path.exists (PHPCGI_PATH)
