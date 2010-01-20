import CTK

# http://stackoverflow.com/questions/1446765/python-cgi-fieldstorage-too-slow
# http://tools.cherrypy.org/wiki/DirectToDiskFileUpload


class default:
    def __init__ (self):
        self.page  = CTK.Page ()
        self.page += CTK.Uploader()

    def __call__ (self):
        return self.page.Render()


tmp = """
import os
import cgi

class MyFieldStorage(cgi.FieldStorage):
    def make_file (self, binary=None):
        print "~~~~~> %s <~~~~~~" %(self.filename)
        return open(os.path.join('/tmp', self.filename), 'wb')

def upload():
    scgi = CTK.request.scgi

    print "Reading.."
    form = MyFieldStorage (fp=scgi.rfile, environ=scgi.env, keep_blank_values=1)

    return "ok"
"""

CTK.publish ('', default)
#CTK.publish ("/upload", upload)
CTK.run (port=8000)
