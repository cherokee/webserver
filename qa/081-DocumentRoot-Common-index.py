from base import *

MAGIC1 = "First part"
MAGIC2 = "Second part"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name              = "DocumentRoot: common, index file"
        self.request           = "GET /dr_common_index/test_index.php HTTP/1.0\r\n" 
        self.expected_error    = 200
        self.expected_content  = MAGIC1 + MAGIC2
        self.forbidden_content = ["<?php", "Index of"]

    def Prepare (self, www):
        path = self.Mkdir (www, "dr_common_index/in/si/de")
        self.WriteFile (www, "dr_common_index/in/si/de/test_index.php", 0444,
                        '<?php echo "%s"."%s"; ?>' % (MAGIC1, MAGIC2))

        self.conf = """
              Directory /dr_common_index {
                 Handler common
                 DocumentRoot %s
              }
              """ % (path)


    def Precondition (self):
        return os.path.exists (PHPCGI_PATH)


