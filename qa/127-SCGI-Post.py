import os
from base import *

DIR    = "/SCGI2/"
MAGIC  = "Cherokee and SCGI rocks!"
PORT   = get_free_port()
PYTHON = look_for_python()

SCRIPT = """
from pyscgi import *

class TestHandler (SCGIHandler):
    def handle_request (self):
        self.handle_post()
        self.send('Content-Type: text/plain\\r\\n\\r\\n')
        self.send('Post: %%s\\n' %% (self.post))

SCGIServer(TestHandler, port=%d).serve_forever()
""" % (PORT)

source = get_next_source()

CONF = """
vserver!1!rule!1270!match = directory
vserver!1!rule!1270!match!directory = %(DIR)s
vserver!1!rule!1270!handler = scgi
vserver!1!rule!1270!handler!balancer = round_robin
vserver!1!rule!1270!handler!balancer!source!1 = %(source)d

source!%(source)d!type = interpreter
source!%(source)d!host = localhost:%(PORT)d
source!%(source)d!interpreter = %(PYTHON)s %(scgi_file)s
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
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

        vars = globals()
        vars['scgi_file'] = scgi_file
        self.conf = CONF % (vars)
