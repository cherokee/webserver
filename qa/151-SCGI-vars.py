import os
from base import *

DIR    = "/SCGI5"
PORT   = get_free_port()
PYTHON = look_for_python()

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
vserver!1!rule!1510!match = directory
vserver!1!rule!1510!match!directory = %(DIR)s
vserver!1!rule!1510!handler = scgi
vserver!1!rule!1510!handler!check_file = 0
vserver!1!rule!1510!handler!balancer = round_robin
vserver!1!rule!1510!handler!balancer!source!1 = %(source)d

source!%(source)d!type = interpreter
source!%(source)d!host = localhost:%(PORT)d
source!%(source)d!interpreter = %(PYTHON)s %(scgi_file)s
"""

EXPECTED = [
    'PATH_INFO: "/"',
    'SCRIPT_NAME: "%s"' % (DIR)
]

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "SCGI V: PATH_INFO and SCRIPT_NAME"

        self.request           = "GET %s/ HTTP/1.0\r\n" %(DIR)
        self.expected_error    = 200
        self.expected_content  = EXPECTED
        self.forbidden_content = ['pyscgi', 'SCGIServer', 'write']

    def Prepare (self, www):
        scgi_file = self.WriteFile (www, "scgi_test5.scgi", 0444, SCRIPT)

        pyscgi = os.path.join (www, 'pyscgi.py')
        if not os.path.exists (pyscgi):
            self.CopyFile ('pyscgi.py', pyscgi)

        vars = globals()
        vars['scgi_file'] = scgi_file
        self.conf = CONF % (vars)
