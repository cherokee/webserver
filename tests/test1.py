import CTK

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

def apply (post):
    return {'ret': "ok"}

class default:
    def __init__ (self):
        a = CTK.PropsTableAuto ('/apply')
        a.Add ('Name',    CTK.TextField({'name': "server!uno"}),    'Example 1')
        a.Add ('Surname', CTK.TextField({'name': "server!dos"}),    'Lalala')
        a.Add ('Nick',    CTK.TextField({'name': "server!tri"}),    'Oh uh ah!')
        a.Add ('Active',  CTK.Checkbox ({'name': "server!active"}), 'Nuevo')

        b = CTK.PropsTableAuto ('/apply')
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
