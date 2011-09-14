from base import *

DIR      = "sec_download_3"
DIR_REAL = "real_205"

SECRET  = "Alvaro_alobbs.com"
MAGIC   = str_random (100)
FILE    = "filename.ext"
TIMEOUT = 7 * 24 * 60 * 60  # 1 week

CONF = """
vserver!1!rule!2050!match = directory
vserver!1!rule!2050!match!directory = /%s
vserver!1!rule!2050!handler = secdownload
vserver!1!rule!2050!handler!secret = %s
vserver!1!rule!2030!handler!timeout = %s
vserver!1!rule!2050!document_root = %s
"""

def secure_download (url, secret):
    import time
    try:
        from hashlib import md5
    except ImportError:
        from md5 import md5

    t = "%08x"%(time.time())
    md5 = '/'+ md5(secret + url + t).hexdigest() +'/'+ t + url
    return md5[10:]+'00'+md5[12:]

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Secure Download: Bad MD5"
        req = '/%s%s' %(DIR, secure_download('/%s'%(FILE), SECRET))
        self.request          = "GET %s HTTP/1.0\r\n" % (req)
        self.expected_error   = 404

    def Prepare (self, www):
        droot = self.Mkdir (www, DIR_REAL)
        self.WriteFile (droot, FILE, 0444, MAGIC)

        self.conf = CONF % (DIR, SECRET, TIMEOUT, droot)
