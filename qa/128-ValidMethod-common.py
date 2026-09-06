from qa_base import *

DIR   = "common_valid_methods1"
MAGIC = "Report bugs to http://bugs.cherokee-project.com"

CONF = """
vserver!1!rule!1280!match = directory
vserver!1!rule!1280!match!directory = /%s
vserver!1!rule!1280!handler = common
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Common: Valid methods, Post"

        self.request           = f"POST /{DIR}/?arg=1 HTTP/1.0\r\n" +\
                                 "Content-type: application/x-www-form-urlencoded\r\n" +\
                                 f"Content-length: {int(len(MAGIC) + 4)}\r\n"
        self.post              = "var="+MAGIC
        self.expected_error    = 200
        self.expected_content  = "Post: "+MAGIC
        self.conf              = CONF % (DIR)

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, "test_index.php", 0o444,
                            "<?php echo 'Post: '.$_POST['var']; ?>")

    def Precondition (self):
        return os.path.exists (look_for_php())
