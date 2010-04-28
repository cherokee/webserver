from base import *

DIR      = "sec_download_1"
DIR_REAL = "real_203"

SECRET = "Alvaro_alobbs.com"
MAGIC  = str_random (100)
FILE   = "filename.ext"

CONF = """
vserver!1!rule!2030!match = directory
vserver!1!rule!2030!match!directory = /%s
vserver!1!rule!2030!handler = secdownload
vserver!1!rule!2030!handler!secret = %s
vserver!1!rule!2030!document_root = %s
"""

def secure_download (url, secret):
    import time
    try:
        from hashlib import md5
    except ImportError:
        from md5 import md5

    t = "%08x"%(time.time())
    return '/'+ md5(secret + url + t).hexdigest() +'/'+ t + url

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Secure Download"
        req = '/%s%s' %(DIR, secure_download('/%s'%(FILE), SECRET))
        self.request          = "GET %s HTTP/1.0\r\n" % (req)
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        droot = self.Mkdir (www, DIR_REAL)
        self.WriteFile (droot, FILE, 0444, MAGIC)

        self.conf = CONF % (DIR, SECRET, droot)
