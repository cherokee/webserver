from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name           = "Inherit dir config"
        self.request        = "GET /inherit1/dir1/dir2/dir3/test HTTP/1.0\r\n"
        self.expected_error = 401

    def Prepare (self, www):
        self.Mkdir (www, "inherit1/dir1/dir2/dir3")
        fn = self.WriteFile (www, "inherit1/dir1/dir2/dir3/test", 0555, "content")

        self.conf = """
              Directory /inherit1 {
                  Auth Basic {
                       Name "Test"
                       Method plain { PasswdFile %s }
                  }
              }

              Directory /inherit1/dir1/dir2/dir3 {
                 Handler file
              }
              """ % (fn)
