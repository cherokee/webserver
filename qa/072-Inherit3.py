from base import *

COMMENT = "/* This is a PHP comment */"
MAGIC   = "This is th magic string"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name              = "Inherit config: deeper fist"
        self.request           = "GET /inherit3/dir1/dir2/dir3/deeper.php HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = COMMENT
        self.conf              = """
              Directory /inherit3 {
                 Handler file
              }

              Directory /inherit3/dir1/dir2/dir3 {
                  Handler phpcgi {
                    Interpreter %s
                  }
              }
              """ % (PHPCGI_PATH)

    def Prepare (self, www):
        self.Mkdir (www, "inherit3/dir1/dir2/dir3")
        self.WriteFile (www, "inherit3/dir1/dir2/dir3/deeper.php", 0444,
                        '<?php %s echo "%s"; ?>'%(COMMENT,MAGIC))

    def Precondition (self):
        return os.path.exists (PHPCGI_PATH)

