from base import *

DIR       = "pathinfo2/deep/deep"
PATH_INFO = "/param1/param2/param3"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "PathInfo, php"
        self.request          = "GET /%s/test.php%s HTTP/1.0\r\n" %(DIR, PATH_INFO)
        self.expected_error   = 200
        self.expected_content = "PathInfo is: "+PATH_INFO

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test.php", 0444,
                        '<?php echo "PathInfo is: ".$_SERVER[\'PATH_INFO\']; ?>')

    def Precondition (self):
        return os.path.exists (look_for_php())
