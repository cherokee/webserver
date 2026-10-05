from qa_base import *

MAGIC = "This_is_the_magic_key"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "PHP get"
        self.request          = f"GET /php4/get.php?this=1&magic={MAGIC} HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "php4")
        self.WriteFile (www, "php4/get.php", 0o444, '<?php echo $_GET["magic"] ?>')

    def Precondition (self):
        return os.path.exists (look_for_php())
