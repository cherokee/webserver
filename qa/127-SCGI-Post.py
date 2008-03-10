import os
from base import *

DIR   = "/SCGI2/"
MAGIC = "Cherokee and SCGI rocks!"
PORT  = get_free_port()

SCRIPT = """
from pyscgi import *

class TestHandler (SCGIHandler):
    def handle_request (self):
        self.handle_post()
        self.output.write('Content-Type: text/plain\\r\\n\\r\\n')
        self.output.write('Post: %%s\\n' %% (self.post))

SCGIServer(TestHandler, port=%d).serve_forever()
""" % (PORT)

CONF = """
vserver!default!directory!<dir>!handler = scgi
vserver!default!directory!<dir>!handler!balancer = round_robin
vserver!default!directory!<dir>!handler!balancer!type = interpreter
vserver!default!directory!<dir>!handler!balancer!local_scgi2!host = localhost:%d
vserver!default!directory!<dir>!handler!balancer!local_scgi2!interpreter = %s %s
vserver!default!directory!<dir>!priority = 1270
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "SCGI II: Post"

        self.request           = "POST %s HTTP/1.0\r\n" %(DIR) +\
                                 "Content-type: application/x-www-form-urlencoded\r\n" +\
                                 "Content-length: %d\r\n" % (len(MAGIC))
        self.post              = MAGIC
        self.expected_error    = 200
        self.expected_content  = "Post: "+MAGIC
        self.forbidden_content = ["pyscgi", "SCGIServer", "write"]

    def Prepare (self, www):
        scgi_file = self.WriteFile (www, "scgi_test2.scgi", 0444, SCRIPT)

        pyscgi = os.path.join (www, 'pyscgi.py')
        if not os.path.exists (pyscgi):
            self.CopyFile ('pyscgi.py', pyscgi)

        self.conf = CONF % (PORT, look_for_python(), scgi_file)
        self.conf = self.conf.replace ('<dir>', DIR)
