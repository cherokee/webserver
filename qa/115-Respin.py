from base import *

MAGIC = "Cherokee_is_pure_magic"

CONF = """
vserver!default!directory!/respin1!handler = common
vserver!default!directory!/respin1!priority = 1150

vserver!default!directory!/respin1-cgi!handler = phpcgi
vserver!default!directory!/respin1-cgi!handler!interpreter = %s
vserver!default!directory!/respin1-cgi!priority = 1151

vserver!default!request!/respin1/.+/!handler = redir
vserver!default!request!/respin1/.+/!handler!rewrite!1!show = 0
vserver!default!request!/respin1/.+/!handler!rewrite!1!regex = ^/respin1/(.+)/$
vserver!default!request!/respin1/.+/!handler!rewrite!1!substring = /respin1-cgi/file?param=$1
vserver!default!request!/respin1/.+/!priority = 1152
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Redirection to PHP"

        self.request           = "GET /respin1/%s/ HTTP/1.0\r\n" % (MAGIC)
        self.expected_error    = 200        
        self.expected_content  = "param is %s" % (MAGIC)
        self.conf              = CONF % (look_for_php())

    def Prepare (self, www):
        d = self.Mkdir (www, "respin1-cgi")
        self.WriteFile (d, "file", 0444,
                        '<?php echo "param is ". $_GET["param"]; ?>')

    def Precondition (self):
        return os.path.exists (look_for_php())
