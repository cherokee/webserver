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
        self.p = CTK.PropsTableAuto ('/apply')
        self.p.Add ('Name',    CTK.TextField({'name': "server!uno"}),    'Example 1')
        self.p.Add ('Surname', CTK.TextField({'name': "server!dos"}),    'Lalala')
        self.p.Add ('Nick',    CTK.TextField({'name': "server!tri"}),    'Oh uh ah!')
        self.p.Add ('Active',  CTK.Checkbox ({'name': "server!active"}), 'Nuevo')
        self.p.Add ('Elige',   CTK.Combobox ({'name': "server!elec", 'selected': "two"}, OPTIONS), 'la lista')
        self.p.Add ('iPhone',  CTK.iPhoneToggle({'name': "server!off"}), 'Fancy')
        self.p.Add ('Carga',   CTK.Proxy("www.cherokee-project.com", '/dynamic/cherokee-list.html'), 'Lista')

    def __call__ (self):
        page = CTK.Page ()
        page += self.p
        return page.Render()


CTK.publish ('', default)
CTK.publish ('^/apply$', apply, method='POST', validation=VALIDATIONS)

CTK.run (port=8000)
