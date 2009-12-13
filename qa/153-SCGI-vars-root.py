import os
from base import *

DIR      = "/Dir153"
PATHINFO = "/dir1/dir2/file.ext"
REQUEST  = DIR + PATHINFO

PORT     = get_free_port()
PYTHON   = look_for_python()

SCRIPT = """
from pyscgi import *

class TestHandler (SCGIHandler):
    def handle_request (self):
        self.handle_post()
        self.send('Content-Type: text/plain\\r\\n\\r\\n')

        for v in self.env:
            self.send('%%s: "%%s"\\n' %% (v, self.env[v]))

SCGIServer(TestHandler, port=%d).serve_forever()
""" % (PORT)

source = get_next_source()

CONF = """
vserver!1530!nick = scgi6
vserver!1530!document_root = /fake

vserver!1530!rule!10!match = default
vserver!1530!rule!10!handler = scgi
vserver!1530!rule!10!handler!check_file = 0
vserver!1530!rule!10!handler!balancer = round_robin
vserver!1530!rule!10!handler!balancer!source!1 = %(source)d

source!%(source)d!type = interpreter
source!%(source)d!host = localhost:%(PORT)d
source!%(source)d!interpreter = %(PYTHON)s %(scgi_file)s
"""

EXPECTED = [
    'PATH_INFO: "%s"' % (DIR + PATHINFO),
    'SCRIPT_NAME: ""'
]

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "SCGI VII: root: PATH_INFO & SCRIPT_NAME"

        self.request           = "GET %s HTTP/1.1\r\n" %(REQUEST) +\
                                 "Host: scgi6\r\n"                +\
                                 "Connection: Close\r\n"

        self.expected_error    = 200
        self.expected_content  = EXPECTED
        self.forbidden_content = ['pyscgi', 'SCGIServer', 'write']

    def Prepare (self, www):
        scgi_file = self.WriteFile (www, "scgi_test7.scgi", 0444, SCRIPT)

        pyscgi = os.path.join (www, 'pyscgi.py')
        if not os.path.exists (pyscgi):
            self.CopyFile ('pyscgi.py', pyscgi)

        vars = globals()
        vars['scgi_file'] = scgi_file
        self.conf = CONF % (vars)
