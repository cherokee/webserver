import os
from qa_base import *
from qa_util import *

DIR    = "/SCGI4/"
PORT   = get_free_port()
PYTHON = look_for_python()

HDR1 = "X-Whatever"
VAL1 = "Value1"

HDR2 = "Something"
VAL2 = "Second"

SCRIPT = f"""
from pyscgi import *

class TestHandler (SCGIHandler):
    def handle_request (self):
        self.handle_post()
        self.send('Content-Type: text/plain\\r\\n\\r\\n')

        for v in self.env:
            self.send(f'{{v}}: {{self.env[v]}}\\n')

SCGIServer(TestHandler, port={PORT}).serve_forever()
"""

source = get_next_source()

CONF = """
vserver!1!rule!1420!match = directory
vserver!1!rule!1420!match!directory = %(DIR)s
vserver!1!rule!1420!handler = scgi
vserver!1!rule!1420!handler!pass_req_headers = 1
vserver!1!rule!1420!handler!balancer = round_robin
vserver!1!rule!1420!handler!balancer!source!1 = %(source)d

source!%(source)d!type = interpreter
source!%(source)d!host = localhost:%(PORT)d
source!%(source)d!interpreter = %(PYTHON)s %(scgi_file)s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "SCGI IV: Extra variables"

        self.request           = f"GET {DIR} HTTP/1.0\r\n" + \
                                 f"{HDR1}: {VAL1}\r\n"  + \
                                 f"{HDR2}: {VAL2}\r\n"
        self.expected_error    = 200
        self.expected_content  = [f'{get_forwarded_http_header(HDR1)}: {VAL1}',
                                  f'{get_forwarded_http_header(HDR2)}: {VAL2}']
        self.forbidden_content = ['pyscgi', 'SCGIServer', 'write']

    def Prepare (self, www):
        scgi_file = self.WriteFile (www, "scgi_test4.scgi", 0o444, SCRIPT)

        pyscgi = os.path.join (www, 'pyscgi.py')
        if not os.path.exists (pyscgi):
            self.CopyFile ('pyscgi.py', pyscgi)

        vars = globals()
        vars['scgi_file'] = scgi_file
        self.conf = CONF % (vars)
