from base import *

FILES   = ["file1", "file2", "file3", "file4"]
DFILES  = ["del1", "del2"]
DIRS    = ["dir1", "dir2", "dir3", "dir4"]
LINKS   = {"link1": "dir1", "link2": "file1" }
BLINKS  = {"broken2": "del1", "broken1": "del2"}

CONF = """
vserver!1!rule!930!match = directory
vserver!1!rule!930!match!directory = /brokenlinks1
vserver!1!rule!930!handler = dirlist
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Broken links"

        self.request           = "GET /brokenlinks1/ HTTP/1.0\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.expected_content  = FILES + DIRS + LINKS.keys()

    def Prepare (self, www):
        base = self.Mkdir (www, "brokenlinks1")

        for filename in FILES + DFILES:
            self.WriteFile (base, filename, 0444, "")

        for dirname in DIRS:
            self.Mkdir (base, dirname)

        for linkname in LINKS:
            os.symlink (os.path.join(base, LINKS[linkname]), os.path.join(base, linkname))

        for linkname in BLINKS:
            os.symlink (os.path.join(base, BLINKS[linkname]), os.path.join(base, linkname))

        for filename in DFILES:
            os.unlink (os.path.join(base,filename))
