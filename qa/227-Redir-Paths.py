from base import *

SERVER  = "redir_paths_1"
MAGIC   = 'Reproducing bug report #432'

CONF = """
vserver!2270!nick = %s
vserver!2270!document_root = %s

vserver!2270!rule!1!match = default
vserver!2270!rule!1!handler = file

vserver!2270!rule!10!match = request
vserver!2270!rule!10!match!request = file.*
vserver!2270!rule!10!handler = redir
vserver!2270!rule!10!handler!rewrite!1!show = 0
vserver!2270!rule!10!handler!rewrite!1!regex = (.*)
vserver!2270!rule!10!handler!rewrite!1!substring = internal.txt
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Internal Redir: Paths"
        self.request          = "GET /in/file.foo HTTP/1.1\r\n" + \
                                "Host: %s\r\n" % (SERVER)       + \
                                "Connection: Close\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d  = self.Mkdir (www, "%s_droot/in"%(SERVER))
        self.WriteFile (d, "internal.txt", 0444, MAGIC)

        self.conf = CONF % (SERVER, d)

