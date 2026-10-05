from qa_base import *

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
    from hashlib import md5

    t = f"{time.time():08x}"
    digest = '/'+ md5((secret + url + t).encode()).hexdigest() +'/'+ t + url
    return digest[10:]+'00'+digest[12:]

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Secure Download: Bad MD5"
        req = '/{}{}'.format(DIR, secure_download('/{}'.format(FILE), SECRET))
        self.request          = f"GET {req} HTTP/1.0\r\n"
        self.expected_error   = 404

    def Prepare (self, www):
        droot = self.Mkdir (www, DIR_REAL)
        self.WriteFile (droot, FILE, 0o444, MAGIC)

        self.conf = CONF % (DIR, SECRET, TIMEOUT, droot)
