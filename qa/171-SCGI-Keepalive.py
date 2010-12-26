import os
from base import *

DIR    = "/SCGI_Keepalive1"
MAGIC  = "It is a kind of magic.. :-)"
PORT   = get_free_port()
PYTHON = look_for_python()

SCRIPT = """
from pyscgi import *

class TestHandler (SCGIHandler):
    def handle_request (self):
        self.handle_post()
        self.send('Content-Length: %d\\r\\n')
        self.send('Content-Type: text/plain\\r\\n\\r\\n')
        self.send('%s')

SCGIServer(TestHandler, port=%d).serve_forever()
""" % (len(MAGIC), MAGIC, PORT)

source = get_next_source()

CONF = """
vserver!1!rule!1710!match = directory
vserver!1!rule!1710!match!directory = %(DIR)s
vserver!1!rule!1710!handler = scgi
vserver!1!rule!1710!handler!check_file = 0
vserver!1!rule!1710!handler!balancer = round_robin
vserver!1!rule!1710!handler!balancer!source!1 = %(source)d

source!%(source)d!type = interpreter
source!%(source)d!host = localhost:%(PORT)d
source!%(source)d!interpreter = %(PYTHON)s %(scgi_file)s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "SCGI Keepalive"

        self.request = "GET %s/ HTTP/1.1\r\n" %(DIR)      + \
                       "Host: localhost\r\n"              + \
                       "Connection: Keep-Alive\r\n\r\n"
        # Second request
        self.request += self.request

        self.expected_error    = 200
        self.expected_content  = [MAGIC]
        self.forbidden_content = ['close', 'pyscgi', 'SCGIServer', 'write']

    def Prepare (self, www):
        scgi_file = self.WriteFile (www, "scgi_keepalive1.scgi", 0444, SCRIPT)

        pyscgi = os.path.join (www, 'pyscgi.py')
        if not os.path.exists (pyscgi):
            self.CopyFile ('pyscgi.py', pyscgi)

        vars = globals()
        vars['scgi_file'] = scgi_file
        self.conf = CONF % (vars)
