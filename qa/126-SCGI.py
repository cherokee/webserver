import os
from base import *

DIR   = "/SCGI1/"
MAGIC = "Cherokee and SCGI rocks!"
PORT  = get_free_port()

SCRIPT = """
from pyscgi import *

class TestHandler (SCGIHandler):
    def handle_request (self):
        self.output.write('Content-Type: text/plain\\r\\n\\r\\n')
        self.output.write('%s\\n')
        
SCGIServerFork(TestHandler, port=%d).serve_forever()
""" % (MAGIC, PORT)

CONF = """
vserver!default!rule!1260!match = directory
vserver!default!rule!1260!match!directory = <dir>
vserver!default!rule!1260!handler = scgi
vserver!default!rule!1260!handler!balancer = round_robin
vserver!default!rule!1260!handler!balancer!type = interpreter
vserver!default!rule!1260!handler!balancer!local_scgi1!host = localhost:%d
vserver!default!rule!1260!handler!balancer!local_scgi1!interpreter = %s %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "SCGI I"

        self.request           = "GET %s HTTP/1.0\r\n" %(DIR)
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["pyscgi", "SCGIServer", "write"]

    def Prepare (self, www):
        scgi_file = self.WriteFile (www, "scgi_test1.scgi", 0444, SCRIPT)

        pyscgi = os.path.join (www, 'pyscgi.py')
        if not os.path.exists (pyscgi):
            self.CopyFile ('pyscgi.py', pyscgi)

        self.conf = CONF % (PORT, look_for_python(), scgi_file)
        self.conf = self.conf.replace ('<dir>', DIR)
