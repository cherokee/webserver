from qa_base import *
from qa_util import *

DIR = "test_271"
NUM = "2250738585072011"

# Ilustrates how to implement a rule to protect PHP and Java back-ends
# against the 2.2250738585072011e-308 bug:
#
# http://www.exploringbinary.com/php-hangs-on-numeric-value-2-2250738585072011e-308/
# http://www.exploringbinary.com/java-hangs-when-converting-2-2250738585072012e-308/
#

CONF = f"""
vserver!1!rule!2710!match = header
vserver!1!rule!2710!match!match = {globals()['NUM']}
vserver!1!rule!2710!match!complete = 1
vserver!1!rule!2710!handler = custom_error
vserver!1!rule!2710!handler!error = 422
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name           = "Full header regex match 1"
        self.request        = f"GET /{globals()['DIR']}/ HTTP/1.0\r\n" + \
                              f"Accept-Language: es, en-gb;q=2.{globals()['NUM']}e-308, en;q=0.7\r\n"
        self.conf           = CONF
        self.expected_error = 422
