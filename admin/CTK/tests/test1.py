import CTK
import time

def error(x):
    raise Exception("Bad dog!!!!")

def ok(x):
    return x.upper()

VALIDATIONS = [
    ("server!uno", ok),
    ("server!dos", error),
    ("server!dos", lambda x: None)
]

URL     = "http://www.cherokee-project.com/dynamic/cherokee-list.html"
OPTIONS = [('one','uno'), ('two','dos'), ('three', 'tres')]

def apply():
    if CTK.post['server!tri']:
        time.sleep(2)

    return {'ret': "ok"}

class default:
    def __init__ (self):
        a = CTK.PropsAuto ('/apply')
        a.Add ('To upcase',   CTK.TextField({'name': "server!uno"}),  'Converts the content of the field to upcase')
        a.Add ('Shows error', CTK.TextField({'name': "server!dos"}),  'It shows an error, it does not matter what you write')
        a.Add ('Delay 2secs', CTK.TextField({'name': "server!tri"}),  'It delays response for 2 seconds, so the submitting message is shown')
        a.Add ('Active',      CTK.Checkbox ({'name': "server!active", 'checked':1}), 'It\'s just a plain checkbox. Nothing to see here')

        b = CTK.PropsAuto ('/apply')
        b.Add ('Elige',   CTK.Combobox ({'name': "server!elec", 'selected': "two"}, OPTIONS), 'la lista')
        b.Add ('iPhone',  CTK.iPhoneToggle({'name': "server!off"}), 'Fancy')
        b.Add ('Carga',   CTK.Proxy("www.cherokee-project.com", '/dynamic/cherokee-list.html'), 'Lista')

        self.tab = CTK.Tab()
        self.tab.Add('Primero', a)
        self.tab.Add('Segundo', b)

    def __call__ (self):
        page = CTK.Page ()
        page += self.tab
        return page.Render()


CTK.publish ('', default)
CTK.publish ('^/apply$', apply, method='POST', validation=VALIDATIONS)

CTK.run (port=8000)
