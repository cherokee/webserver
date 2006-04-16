from base import *

MAGIC = "It shouldn't inherit in this case"

CONF = """
vserver!default!directory!/inherit2/dir1/dir2/dir3!auth = plain
vserver!default!directory!/inherit2/dir1/dir2/dir3!auth!methods = basic
vserver!default!directory!/inherit2/dir1/dir2/dir3!auth!realm = Test
vserver!default!directory!/inherit2/dir1/dir2/dir3!auth!passwdfile = %s
vserver!default!directory!/inherit2/dir1/dir2/dir3!priority = 710

vserver!default!directory!/inherit2!handler = file
vserver!default!directory!/inherit2!priority = 711
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Inherit dir config not reverse"
        self.request          = "GET /inherit2/dir1/test HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "inherit2/dir1/dir2/dir3")
        fn = self.WriteFile (www, "inherit2/dir1/test", 0555, MAGIC)

        self.conf = CONF % (fn)
