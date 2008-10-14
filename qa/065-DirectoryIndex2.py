from base import *

MAGIC1 = "This is the content of.."
MAGIC2 = " ..the PHP index file"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Directory indexer, PHP"

        self.request           = "GET /directoryindex2/ HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC1+MAGIC2
        self.forbidden_content = "<?php"

    def Prepare (self, www):
        d = self.Mkdir (www, "directoryindex2")
        self.WriteFile (d, "test_index.php", 0444,
                        '<?php echo "%s"."%s"; ?>'%(MAGIC1, MAGIC2))

    def Precondition (self):
        return os.path.exists (look_for_php())
