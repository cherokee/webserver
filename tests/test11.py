# -*- coding: utf-8 -*-

import os
import CTK

UPLOAD_DIR = "/tmp"

JS = """
$('#%s').bind ('upload_finished', function (event) {
    var tries = 2;
    var url   = "/get/"+ event.filename;

    function try_to_set_image() {
       $.ajax ({type: 'HEAD', url: url,
          success: function (data) {
            $('#image').html ('<img src="' + url + '" />');
          },
          error: function (xhr, ajaxOptions, thrownError) {
            if (tries-- > 0) {
               window.setTimeout (try_to_set_image, 2000);
            }
          }
       });
    }

    try_to_set_image();
});
"""

def handler_complete (filename, target_dir, target_file, params):
    """ Server-Side 'complete' event handler """

    print "I'm python, and this is the uploaded file:"
    print os.popen("ls -l '%s'" %(os.path.join(target_dir, target_file))).read()
    print "And these the parameters", params

    return 'ok'

def get():
    file = CTK.request.url[len('/get/'):]
    return open(os.path.join (UPLOAD_DIR, file)).read()

class default:
    def __init__ (self):
        upload = CTK.AjaxUpload({'handler': handler_complete, 'target_dir': UPLOAD_DIR}, {'var':'foo'})

        self.page = CTK.Page()
        self.page += CTK.RawHTML ("<h1>Image upload</h1>")
        self.page += CTK.RawHTML ('<div id="image"></div>')
        self.page += upload
        self.page += CTK.RawHTML (js = JS%(upload.id))

    def __call__ (self):
        return self.page.Render()


CTK.publish ('/get', get)
CTK.publish ('', default)
CTK.run (port=8000)
