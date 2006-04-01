from base import *

PATH_INFO = "/param1/param2/param3"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "PathInfo, phpcgi"

        self.request           = "GET /pathinfo2/deep/deep/test.php%s HTTP/1.0\r\n" %(PATH_INFO)
        self.conf              = "Directory /pathinfo2 { Handler phpcgi { Interpreter %s }}" % (PHPCGI_PATH)
        self.expected_error    = 200
        self.expected_content  = "PathInfo is: "+PATH_INFO

    def Prepare (self, www):
        self.Mkdir (www, "pathinfo2/deep/deep/")
        self.WriteFile (www, "pathinfo2/deep/deep/test.php", 0444,
                        '<?php echo "PathInfo is: ".$_SERVER[PATH_INFO]; ?>')

    def Precondition (self):
        return os.path.exists (PHPCGI_PATH)
