from base import *

DOMAIN   = "wildcard1"
FILENAME = "test"
MAGIC1   = "This is virtual server wildcard1"
MAGIC2   = "This is virtual server *.wildcard1"

CONF = """
vserver!1390!nick = <domain>
vserver!1390!document_root = %s

vserver!1390!rule!1!match = default
vserver!1390!rule!1!handler = file

vserver!1391!nick = rest_<domain>
vserver!1391!document_root = %s
vserver!1391!match = wildcard
vserver!1391!match!domain!1 = *.<domain>

vserver!1391!rule!1!match = default
vserver!1391!rule!1!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Domain wildcards 1"

        self.request           = "GET /%s HTTP/1.1\r\n" % (FILENAME) +\
                                 "Host: whatever.%s\r\n" % (DOMAIN)  +\
                                 "Connection: Close\r\n"

        self.expected_error    = 200
        self.expected_content  = MAGIC2


    def Prepare (self, www):
        d1 = self.Mkdir (www, "%s_dir1" % (DOMAIN))
        d2 = self.Mkdir (www, "%s_dir2" % (DOMAIN))

        self.WriteFile (d1, FILENAME, 0444, MAGIC1);
        self.WriteFile (d2, FILENAME, 0444, MAGIC2);


        self.conf = CONF % (d1, d2)
        self.conf = self.conf.replace ('<domain>', DOMAIN)
