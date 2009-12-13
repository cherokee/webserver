from base import *

DIR         = "212_dirlist_hidden"
HIDDEN      = ["cherokee1", "cherokee3", "foo.bar"]
FILES       = ["cherokee2", "lost+found"]

CONF = """
vserver!1!rule!2120!match = directory
vserver!1!rule!2120!match!directory = /%s
vserver!1!rule!2120!handler = common
vserver!1!rule!2120!handler!hidden_files = %s
""" % (DIR, ','.join(HIDDEN))


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Dirlist with hidden files"

        self.request           = "GET /%s/ HTTP/1.0\r\n" %(DIR)
        self.expected_error    = 200
        self.conf              = CONF
        self.expected_content  = FILES
        self.forbidden_content = HIDDEN

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        for t in HIDDEN + FILES:
            self.WriteFile (d, t, 0444)
