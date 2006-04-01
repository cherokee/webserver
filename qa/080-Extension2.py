from base import *

REQUIRED  = "This is working! :-)"
FORBIDDEN = "It shouldn't appear in the text"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name              = "Custom extensions, list"
        self.request           = "GET /extension2/test.def HTTP/1.0\r\n" 
        self.expected_error    = 200
        self.expected_content  = REQUIRED
        self.forbidden_content = FORBIDDEN
        self.conf              = """
              Directory /extension2 {
                 Handler file
              }

              Extension abc, def, ghi { 
                 Handler phpcgi {
                   Interpreter %s
                 }
              }              
              """ % (PHPCGI_PATH) 

    def Prepare (self, www):
        self.Mkdir (www, "extension2")
        self.WriteFile (www, "extension2/test.def", 0444,
                        '<?php /* %s */ echo "%s"; ?>' % (FORBIDDEN, REQUIRED))

    def Precondition (self):
        return os.path.exists (PHPCGI_PATH)

