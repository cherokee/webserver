import CTK

URL = "http://ftp.gnome.org/pub/mozilla.org/addons/14455/tunisia-sat_smilies-2.0-fx.xpi"

class default:
    def __call__ (self):
        down    = CTK.Downloader ('download', URL)
        message = CTK.Box(content = CTK.RawHTML(' '))
        button  = CTK.Button('Download')

        down.bind ('error',    f'$("#{message.id}").html("Error: Could not download");')
        down.bind ('finished', f'$("#{message.id}").html("OK: Download Finished");')
        button.bind ('click', down.JS_to_start())

        page  = CTK.Page()
        page += down
        page += button
        page += message

        return page.Render()

CTK.publish ('', default)
CTK.run (port=8000)
