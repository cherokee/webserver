from base import *

MAGIC = "This_is_the_magic_key"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "PHP post"
        self.request          = "POST /php3/post.php HTTP/1.0\r\n" +\
                                "Content-type: application/x-www-form-urlencoded\r\n" +\
                                "Content-length: %d\r\n" % (6+len(MAGIC))
        self.post             = "magic="+MAGIC

        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, "php3")
        self.WriteFile (d, "post.php", 0444, '<?php echo $_POST["magic"] ?>')

    def Precondition (self):
        return os.path.exists (look_for_php())
