import CTK

class default:
    def __init__ (self):
        self.page  = CTK.Page ()
        self.page += CTK.Uploader()

    def __call__ (self):
        return self.page.Render()

CTK.publish ('', default)
CTK.run (port=8000)
