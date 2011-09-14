from base import *

DIR      = "sec_download_2"
DIR_REAL = "real_204"

SECRET  = "Alvaro_alobbs.com"
MAGIC   = str_random (100)
FILE    = "filename.ext"
TIMEOUT = 7 * 24 * 60 * 60  # 1 week

CONF = """
vserver!1!rule!2040!match = directory
vserver!1!rule!2040!match!directory = /%s
vserver!1!rule!2040!handler = secdownload
vserver!1!rule!2040!handler!secret = %s
vserver!1!rule!2030!handler!timeout = %s
vserver!1!rule!2040!document_root = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Secure Download: Gone"
        self.request          = "GET /%s/12345678901234567890123456789012/00000000/a HTTP/1.0\r\n"%(DIR)
        self.expected_error   = 410

    def Prepare (self, www):
        droot = self.Mkdir (www, DIR_REAL)
        self.WriteFile (droot, FILE, 0444, MAGIC)

        self.conf = CONF % (DIR, SECRET, TIMEOUT, droot)
