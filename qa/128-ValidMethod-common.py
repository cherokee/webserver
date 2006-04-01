from base import *

DIR   = "common_valid_methods1"
MAGIC = "Report bugs to http://bugs.0x50.org"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Common: Valid methods, Post"

        self.request           = "POST /%s/?arg=1 HTTP/1.0\r\n" %(DIR) +\
                                 "Content-type: application/x-www-form-urlencoded\r\n" +\
                                 "Content-length: %d\r\n" % (len(MAGIC)+4)
        self.post              = "var="+MAGIC
        self.expected_error    = 200
        self.expected_content  = "Post: "+MAGIC
        self.conf              = """Directory /%s {
                                       Handler common
                                 }""" % (DIR)

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, "test_index.php", 0444,
                            "<?php echo 'Post: '.$_POST['var']; ?>")

    def Precondition (self):
        return os.path.exists (PHPCGI_PATH)

