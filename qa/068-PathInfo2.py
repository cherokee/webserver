from base import *

PATH_INFO = "/param1/param2/param3"

CONF = """
vserver!default!directory!/pathinfo2!handler = phpcgi
vserver!default!directory!/pathinfo2!handler!interpreter = %s
vserver!default!directory!/pathinfo2!priority = 680
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "PathInfo, phpcgi"

        self.request           = "GET /pathinfo2/deep/deep/test.php%s HTTP/1.0\r\n" %(PATH_INFO)
        self.conf              = CONF % (look_for_php())
        self.expected_error    = 200
        self.expected_content  = "PathInfo is: "+PATH_INFO

    def Prepare (self, www):
        self.Mkdir (www, "pathinfo2/deep/deep/")
        self.WriteFile (www, "pathinfo2/deep/deep/test.php", 0444,
                        '<?php echo "PathInfo is: ".$_SERVER[PATH_INFO]; ?>')

    def Precondition (self):
        return os.path.exists (look_for_php())
