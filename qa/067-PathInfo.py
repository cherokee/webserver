from base import *

PATH_INFO = "/param1/param2/param3"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "PathInfo, common"

        self.request           = "GET /pathinfo/test.php%s HTTP/1.0\r\n" %(PATH_INFO)
        self.expected_error    = 200
        self.expected_content  = "PathInfo is: "+PATH_INFO

        self.conf              = """#
                                    # If this test fails is probably due to an error in PHP5:
                                    #
                                    #        http://bugs.php.net/bug.php?id=31892
                                    #
                                    # - Work around: Append the following line to your php
                                    # configuration file, usually /etc/php5/cgi/php.ini:
                                    # 
                                    #    cgi.fix_pathinfo=0
                                    #
                                    Directory /pathinfo { Handler common }
                                 """

    def Prepare (self, www):
        self.Mkdir (www, "pathinfo")
        self.WriteFile (www, "pathinfo/test.php", 0444, '<?php echo "PathInfo is: ".$_SERVER[PATH_INFO]; ?>')

    def Precondition (self):
        return os.path.exists (PHPCGI_PATH)
