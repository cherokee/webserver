from qa_base import *

DIR     = "/deep/inside/bar/foo"
SERVER  = "redir_paths_2"
MAGIC   = 'Reproducing bug report #432'

CONF = """
vserver!2280!nick = %s
vserver!2280!document_root = %s

vserver!2280!rule!1!match = default
vserver!2280!rule!1!handler = file

vserver!2280!rule!10!match = directory
vserver!2280!rule!10!match!directory = %s
vserver!2280!rule!10!handler = redir
vserver!2280!rule!10!handler!rewrite!1!show = 0
vserver!2280!rule!10!handler!rewrite!1!regex = (.*)
vserver!2280!rule!10!handler!rewrite!1!substring = internal.txt
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Internal Redir, Directory match: Paths "
        self.request          = f"GET /{DIR}/file.foo HTTP/1.1\r\n" + \
                                f"Host: {SERVER}\r\n"              + \
                                "Connection: Close\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d  = self.Mkdir (www, f"{SERVER}_droot/{DIR}")
        self.WriteFile (d, "internal.txt", 0o444, MAGIC)

        self.conf = CONF % (SERVER, d, DIR)
