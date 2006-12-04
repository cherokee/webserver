from base import *

REQUIRED  = "This is working, yeahhh"
FORBIDDEN = "It shouldn't appear in the text"

CONF = """
vserver!default!directory!/extension1!handler = file
vserver!default!directory!/extension1!priority = 790

vserver!default!extensions!xyz!handler = phpcgi
vserver!default!extensions!xyz!handler!interpreter = %s
vserver!default!extensions!xyz!priority = 791
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


