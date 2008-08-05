import os
from base import *

DIR      = "/SCGI6"
PATHINFO = "/dir1/dir2/dir3/looongfile.ext"
REQUEST  = "%s%s"%(DIR, PATHINFO)
PORT     = get_free_port()

SCRIPT = """
from pyscgi import *

class TestHandler (SCGIHandler):
    def handle_request (self):
        self.handle_post()
        self.output.write('Content-Type: text/plain\\r\\n\\r\\n')

        for v in self.env:
            self.output.write('%%s: "%%s"\\n' %% (v, self.env[v]))

SCGIServer(TestHandler, port=%d).serve_forever()
""" % (PORT)

CONF = """
vserver!1!rule!1520!match = directory
vserver!1!rule!1520!match!directory = <dir>
vserver!1!rule!1520!handler = scgi
vserver!1!rule!1520!handler!check_file = 0
vserver!1!rule!1520!handler!balancer = round_robin
vserver!1!rule!1520!handler!balancer!type = interpreter
vserver!1!rule!1520!handler!balancer!1!host = localhost:%d
vserver!1!rule!1520!handler!balancer!1!interpreter = %s %s
"""

EXPECTED = [
    'PATH_INFO: "%s"' %(PATHINFO),
    'SCRIPT_NAME: "%s"' % (DIR)
]

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "SCGI VI: PATH_INFO and SCRIPT_NAME 2"

        self.request           = "GET %s HTTP/1.0\r\n" %(REQUEST)
        self.expected_error    = 200
        self.expected_content  = EXPECTED
        self.forbidden_content = ['pyscgi', 'SCGIServer', 'write']

    def Prepare (self, www):
        scgi_file = self.WriteFile (www, "scgi_test6.scgi", 0444, SCRIPT)

        pyscgi = os.path.join (www, 'pyscgi.py')
        if not os.path.exists (pyscgi):
            self.CopyFile ('pyscgi.py', pyscgi)

        self.conf = CONF % (PORT, look_for_python(), scgi_file)
        self.conf = self.conf.replace ('<dir>', DIR)
