from base import *

CONF = """
vserver!1!rule!700!match = directory
vserver!1!rule!700!match!directory = /inherit1
vserver!1!rule!700!auth = plain
vserver!1!rule!700!auth!methods = basic
vserver!1!rule!700!auth!realm = Test
vserver!1!rule!700!auth!passwdfile = %s

vserver!1!rule!701!match = directory
vserver!1!rule!701!match!directory = /inherit1/dir1/dir2/dir3
vserver!1!rule!701!match!final = 0
vserver!1!rule!701!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name           = "Inherit dir config"
        self.request        = "GET /inherit1/dir1/dir2/dir3/test HTTP/1.0\r\n"
        self.expected_error = 401

    def Prepare (self, www):
        self.Mkdir (www, "inherit1/dir1/dir2/dir3")
        fn = self.WriteFile (www, "inherit1/dir1/dir2/dir3/test", 0555, "content")

        self.conf = CONF % (fn)
