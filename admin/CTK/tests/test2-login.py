import CTK
import random

LOGINS = {
    'root': "god",
    'alo' : "cherokee"
}

SECRET = ''.join ([random.choice("1234567890") for x in range(30)])


def welcome():
    if CTK.cookie['secret'] != SECRET:
        return CTK.HTTP_Redir('/')

    dialog = CTK.Dialog ({'title': "You did it"})
    dialog += CTK.RawHTML ("<h1>Welcome Sir!</h1>")

    page = CTK.Page()
    page += dialog
    return page.Render()


def apply():
    name  = CTK.post['login!name']
    passw = CTK.post['login!pass']

    if not name in LOGINS:
        return {'ret': "error", 'errors': {'login!name': "Unknown user"}}
    if LOGINS[name] != passw:
        return {'ret': "error", 'errors': {'login!pass': "Wrong password"}}

    CTK.cookie['secret'] = SECRET
    return {'ret': "ok", 'redirect': "/welcome"}


class default:
    def __init__ (self):
        g = CTK.PropsTable()
        g.Add ('User',     CTK.TextField({'name': "login!name", 'class': "required"}), 'Type your user name')
        g.Add ('Password', CTK.TextFieldPassword({'name': "login!pass", 'class': "required"}), 'Type your password')

        form = CTK.Submitter("/apply")
        form += g

        self.page = CTK.Page()
        self.page += form

    def __call__ (self):
        return self.page.Render()


CTK.publish ('/welcome', welcome)
CTK.publish ('/apply', apply, method="POST")
CTK.publish ('', default)

CTK.run (port=8000)
