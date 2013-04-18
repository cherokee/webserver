import CTK

URL = "http://ftp.gnome.org/pub/mozilla.org/addons/14455/tunisia-sat_smilies-2.0-fx.xpi"

class default:
    def __call__ (self):
        down    = CTK.Downloader (URL)
        message = CTK.Box(content = CTK.RawHTML(' '))
        button  = CTK.Button('Download')

        down.bind ('error',    '$("#%s").html("Error: Could not download");' %(message.id))
        down.bind ('finished', '$("#%s").html("OK: Download Finished");' %(message.id))
        button.bind ('click', down.JS_to_start())

        page  = CTK.Page()
        page += down
        page += button
        page += message

        return page.Render()

CTK.publish ('', default)
CTK.run (port=8000)
