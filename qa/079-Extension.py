from base import *

REQUIRED  = "This is working, yeahhh"
FORBIDDEN = "It shouldn't appear in the text"

CONF = """
vserver!1!rule!790!match = directory
vserver!1!rule!790!match!directory = /extension1
vserver!1!rule!790!handler = file

vserver!1!rule!791!match = extensions
vserver!1!rule!791!match!extensions = xyz
vserver!1!rule!791!handler = phpcgi
vserver!1!rule!791!handler!interpreter = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name              = "Custom extensions"
        self.request           = "GET /extension1/test.xyz HTTP/1.0\r\n" 
        self.expected_error    = 200
        self.expected_content  = REQUIRED
        self.forbidden_content = FORBIDDEN
        self.conf              = CONF % (look_for_php())

    def Prepare (self, www):
        self.Mkdir (www, "extension1")
        self.WriteFile (www, "extension1/test.xyz", 0444,
                        '<?php /* %s */ echo "%s"; ?>' % (FORBIDDEN, REQUIRED))

    def Precondition (self):
        return os.path.exists (look_for_php())


