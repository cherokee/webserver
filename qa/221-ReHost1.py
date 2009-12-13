from base import *

MAGIC = "Testing regex Host: mathing."

CONF = """
vserver!2210!nick = test0221
vserver!2210!document_root = %s
vserver!2210!match = rehost
vserver!2210!match!regex!1 = ^221begin.+end$
vserver!2210!match!regex!2 = ^221test[^.]+\.domain$
vserver!2210!match!regex!3 = ^221foobar.*$
vserver!2210!rule!1!match = default
vserver!2210!rule!1!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Host regex match"
        self.request          = "GET /file HTTP/1.1\r\n" +\
                                "Connection: Close\r\n" + \
                                "Host: 221test000.domain\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, "test_221_dir")
        self.WriteFile (d, "file", 0444, MAGIC)

        self.conf = CONF % (d)
