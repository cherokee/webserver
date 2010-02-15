import os
import CTK

UPLOAD_DIR = "/tmp"

def ok (filename, target_dir, target_file, params):
    txt =  "<h1>It worked!</h1>"
    txt += "<pre>%s</pre>" %(os.popen("ls -l '%s'" %(os.path.join(target_dir, target_file))).read())
    txt += "<p>Params: %s</p>" %(str(params))
    txt += "<p>Filename: %s</p>" %(filename)
    return txt

class default:
    def __init__ (self):
        self.page  = CTK.Page ()

        self.page += CTK.RawHTML ("<h1>Direct Upload with params</h1>")
        self.page += CTK.Uploader({'handler': ok, 'target_dir': UPLOAD_DIR}, {'var':'foo'})

        self.page += CTK.RawHTML ("<h1>Temporal Upload without params</h1>")
        self.page += CTK.Uploader({'handler': ok, 'target_dir': UPLOAD_DIR}, direct=False)

    def __call__ (self):
        return self.page.Render()

CTK.publish ('', default)
CTK.run (port=8000)
