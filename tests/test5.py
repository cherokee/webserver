import CTK

def do_ok():
    return "It worked!"

class default:
    def __init__ (self):
        self.page  = CTK.Page ()
        self.page += CTK.Uploader({'handler': do_ok})

    def __call__ (self):
        return self.page.Render()

CTK.publish ('', default)
CTK.run (port=8000)
