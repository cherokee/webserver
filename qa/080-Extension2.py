from base import *

REQUIRED  = "This is working! :-)"
FORBIDDEN = "It shouldn't appear in the text"

CONF = """
vserver!1!rule!800!match = directory
vserver!1!rule!800!match!directory = /extension2
vserver!1!rule!800!handler = file

vserver!1!rule!801!match = extensions
vserver!1!rule!801!match!extensions = abc,def,ghi
vserver!1!rule!801!handler = phpcgi
vserver!1!rule!801!handler!interpreter = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name              = "Custom extensions, list"
        self.request           = "GET /extension2/test.def HTTP/1.0\r\n" 
        self.expected_error    = 200
        self.expected_content  = REQUIRED
        self.forbidden_content = FORBIDDEN
        self.conf              = CONF % (look_for_php())

    def Prepare (self, www):
        self.Mkdir (www, "extension2")
        self.WriteFile (www, "extension2/test.def", 0444,
                        '<?php /* %s */ echo "%s"; ?>' % (FORBIDDEN, REQUIRED))

    def Precondition (self):
        return os.path.exists (look_for_php())

