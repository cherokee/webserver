from base import *

MAGIC = "It shouldn't inherit in this case"

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

        self.conf = """
              Directory /inherit2 {
                 Handler file
              }

              Directory /inherit2/dir1/dir2/dir3 {
                  Handler common 
                  Auth Basic {
                       Name "Test"
                       Method plain { PasswdFile %s }
                  }
              }
              """ % (fn)
