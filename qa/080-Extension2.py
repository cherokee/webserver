from base import *

REQUIRED  = "This is working! :-)"
FORBIDDEN = "It shouldn't appear in the text"

CONF = """
vserver!default!directory!/extension2!handler = file
vserver!default!directory!/extension2!priority = 800

vserver!default!extensions!abc,def,ghi!handler = phpcgi
vserver!default!extensions!abc,def,ghi!handler!interpreter = %s
vserver!default!extensions!abc,def,ghi!priority = 801
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

