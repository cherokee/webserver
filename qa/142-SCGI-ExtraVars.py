import os
from base import *

DIR  = "/SCGI4/"
PORT = get_free_port()

HDR1 = "X-Whatever"
VAL1 = "Value1"

HDR2 = "Something"
VAL2 = "Second"

SCRIPT = """
from pyscgi import *

class TestHandler (SCGIHandler):
    def handle_request (self):
        self.handle_post()
        self.output.write('Content-Type: text/plain\\r\\n\\r\\n')

        for v in self.env:
            self.output.write('%%s: %%s\\n' %% (v, self.env[v]))

SCGIServer(TestHandler, port=%d).serve_forever()
""" % (PORT)

CONF = """
vserver!default!directory!<dir>!handler = scgi
vserver!default!directory!<dir>!handler!pass_req_headers = 1
vserver!default!directory!<dir>!handler!balancer = round_robin
vserver!default!directory!<dir>!handler!balancer!type = interpreter
vserver!default!directory!<dir>!handler!balancer!local_scgi4!host = localhost:%d
vserver!default!directory!<dir>!handler!balancer!local_scgi4!interpreter = %s %s
vserver!default!directory!<dir>!priority = 1420
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "SCGI IV: Extra variables"

        self.request           = "GET %s HTTP/1.0\r\n" %(DIR) + \
                                 "%s: %s\r\n" % (HDR1, VAL1)  + \
                                 "%s: %s\r\n" % (HDR2, VAL2)
        self.expected_error    = 200
        self.expected_content  = ['%s: %s'%(HDR1, VAL1), '%s: %s'%(HDR2, VAL2)]
        self.forbidden_content = ['pyscgi', 'SCGIServer', 'write']

    def Prepare (self, www):
        scgi_file = self.WriteFile (www, "scgi_test4.scgi", 0444, SCRIPT)

        pyscgi = os.path.join (www, 'pyscgi.py')
        if not os.path.exists (pyscgi):
            self.CopyFile ('pyscgi.py', pyscgi)

        self.conf = CONF % (PORT, look_for_python(), scgi_file)
        self.conf = self.conf.replace ('<dir>', DIR)
