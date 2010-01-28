import os
from base import *

MAGIC = "Cherokee Rocks!"

CONF = """
vserver!2440!nick = directoryoverwrite
vserver!2440!document_root = %s
vserver!2440!directory_index = index.html
vserver!2440!rule!300!document_root = %s
vserver!2440!rule!300!match = directory
vserver!2440!rule!300!match!directory = /inside
vserver!2440!rule!300!match!final = 0
vserver!2440!rule!100!handler = common
vserver!2440!rule!100!handler!iocache = 0
vserver!2440!rule!100!match = default
vserver!2440!rule!100!match!final = 1
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name           = "Rule: Directory properties overwrite"
        self.request        = "GET /inside/ HTTP/1.0\r\n" +\
                              "Host: directoryoverwrite\r\n"
        self.expected_error = 200

    def Prepare (self, www):
        # Generate files and dir
        self.vsdr = self.Mkdir (www, "directoryoverwrite")
        self.rldr = self.Mkdir (www, "directoryinside")

        self.WriteFile (self.rldr, "index.html", 0666, MAGIC)

        # Set the configuration
        self.conf = CONF % (self.vsdr, self.rldr)

        # Expected output
        self.expected_content = [MAGIC]
