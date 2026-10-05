import os
from qa_base import *

DIR    = "/SCGI1/"
MAGIC  = "Cherokee and SCGI rocks!"
PORT   = get_free_port()
PYTHON = look_for_python()

SCRIPT = f"""
from pyscgi import *

class TestHandler (SCGIHandler):
    def handle_request (self):
        self.send('Content-Type: text/plain\\r\
\\r\
')
        self.send('{MAGIC}\
')

SCGIServerFork(TestHandler, port={int(PORT)}).serve_forever()
"""

source = get_next_source()

CONF = """
vserver!1!rule!1260!match = directory
vserver!1!rule!1260!match!directory = %(DIR)s
vserver!1!rule!1260!handler = scgi
vserver!1!rule!1260!handler!balancer = round_robin
vserver!1!rule!1260!handler!balancer!source!1 = %(source)d

source!%(source)d!type = interpreter
source!%(source)d!host = localhost:%(PORT)d
source!%(source)d!interpreter = %(PYTHON)s %(scgi_file)s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "SCGI I"

        self.request           = f"GET {DIR} HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["pyscgi", "SCGIServer", "write"]

    def Prepare (self, www):
        scgi_file = self.WriteFile (www, "scgi_test1.scgi", 0o444, SCRIPT)

        pyscgi = os.path.join (www, 'pyscgi.py')
        if not os.path.exists (pyscgi):
            self.CopyFile ('pyscgi.py', pyscgi)

        vars = globals()
        vars['scgi_file'] = scgi_file
        self.conf = CONF % (vars)
