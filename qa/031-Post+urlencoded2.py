from base import *

CONF = """
vserver!default!directory!/post3!handler = common
vserver!default!directory!/post3!priority = 310
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Post request"

        self.expected_error = 200
        self.conf           = CONF
        self.request        = "POST /post3/test.php HTTP/1.1\r\n" +\
                              "Host: localhost\r\n" +\
                              "Content-type: application/x-www-form-urlencoded\r\n" +\
                              "Content-length: 8\r\n"
        self.post           = "12345678"

    def Prepare (self, www):
        self.Mkdir (www, "post3")
        self.WriteFile (www, "post3/test.php", 0444, "<?php echo $_POST; ?>")
                        
    def Precondition (self):
        return os.path.exists (look_for_php())

