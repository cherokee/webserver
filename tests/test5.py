import os
import CTK

UPLOAD_DIR = "/tmp"

def ok (filename):
    txt =  "<h1>It worked!</h1>"
    txt += "<pre>%s</pre>" %(os.popen("ls -l '%s/%s'"%(UPLOAD_DIR, filename)).read())
    return txt

class default:
    def __init__ (self):
        self.page  = CTK.Page ()
        self.page += CTK.Uploader({'handler': ok, 'target_dir': UPLOAD_DIR})
        self.page += CTK.Uploader({'handler': ok, 'target_dir': UPLOAD_DIR})

    def __call__ (self):
        return self.page.Render()

CTK.publish ('', default)
CTK.run (port=8000)
