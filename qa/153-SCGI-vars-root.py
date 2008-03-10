import os
from base import *

PORT    = get_free_port()
REQUEST = "/dir1/dir2/file.ext"

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
vserver!scgi6!document_root = /fake
vserver!scgi6!directory!/!handler = scgi
vserver!scgi6!directory!/!handler!check_file = 0
vserver!scgi6!directory!/!handler!balancer = round_robin
vserver!scgi6!directory!/!handler!balancer!type = interpreter
vserver!scgi6!directory!/!handler!balancer!1!host = localhost:%d
vserver!scgi6!directory!/!handler!balancer!1!interpreter = %s %s
vserver!scgi6!directory!/!priority = 1530
"""

EXPECTED = [
    'PATH_INFO: "%s"' % (REQUEST),
    'SCRIPT_NAME: ""' 
]

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "SCGI VII: root: PATH_INFO and SCRIPT_NAME"

        self.request           = "GET %s HTTP/1.1\r\n" %(REQUEST) +\
                                 "Host: scgi6\r\n"
        self.expected_error    = 200
        self.expected_content  = EXPECTED
        self.forbidden_content = ['pyscgi', 'SCGIServer', 'write']

    def Prepare (self, www):
        scgi_file = self.WriteFile (www, "scgi_test7.scgi", 0444, SCRIPT)

        pyscgi = os.path.join (www, 'pyscgi.py')
        if not os.path.exists (pyscgi):
            self.CopyFile ('pyscgi.py', pyscgi)

        self.conf = CONF % (PORT, look_for_python(), scgi_file)
