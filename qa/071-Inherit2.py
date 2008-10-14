from base import *

MAGIC = "It shouldn't inherit in this case"

CONF = """
vserver!1!rule!710!match = directory
vserver!1!rule!710!match!directory = /inherit2/dir1/dir2/dir3
vserver!1!rule!710!auth = plain
vserver!1!rule!710!auth!methods = basic
vserver!1!rule!710!auth!realm = Test
vserver!1!rule!710!auth!passwdfile = %s

vserver!1!rule!711!match = directory
vserver!1!rule!711!match!directory = /inherit2
vserver!1!rule!711!match!final = 0
vserver!1!rule!711!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Inherit dir config not reverse"
        self.request          = "GET /inherit2/dir1/test HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "inherit2/dir1/dir2/dir3")
        fn = self.WriteFile (www, "inherit2/dir1/test", 0555, MAGIC)

        self.conf = CONF % (fn)
