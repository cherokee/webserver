import os
from base import *

DIR   = "/SCGI_Keepalive1"
PORT  = get_free_port()
MAGIC = "It is a kind of magic.. :-)"

SCRIPT = """
from pyscgi import *

class TestHandler (SCGIHandler):
    def handle_request (self):
        self.handle_post()
        self.output.write('Content-Length: %d\\r\\n')
        self.output.write('Content-Type: text/plain\\r\\n\\r\\n')
        self.output.write('%s')

SCGIServer(TestHandler, port=%d).serve_forever()
""" % (len(MAGIC), MAGIC, PORT)

CONF = """
vserver!1!rule!1710!match = directory
vserver!1!rule!1710!match!directory = %s
vserver!1!rule!1710!handler = scgi
vserver!1!rule!1710!handler!check_file = 0
vserver!1!rule!1710!handler!balancer = round_robin
vserver!1!rule!1710!handler!balancer!type = interpreter
vserver!1!rule!1710!handler!balancer!1!host = localhost:%d
vserver!1!rule!1710!handler!balancer!1!interpreter = %s %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "SCGI Keepalive"

        self.request           = "GET %s/ HTTP/1.1\r\n" %(DIR) + \
                                 "Host: localhost\r\n" + \
                                 "Connection: Keep-Alive\r\n\r\n" + \
                                 "OPTIONS %s/ HTTP/1.0\r\n" % (DIR)
        self.expected_error    = 200
        self.expected_content  = [MAGIC, "Connection: Keep-Alive"]
        self.forbidden_content = ['pyscgi', 'SCGIServer', 'write']

    def Prepare (self, www):
        scgi_file = self.WriteFile (www, "scgi_keepalive1.scgi", 0444, SCRIPT)

        pyscgi = os.path.join (www, 'pyscgi.py')
        if not os.path.exists (pyscgi):
            self.CopyFile ('pyscgi.py', pyscgi)

        self.conf = CONF % (DIR, PORT, look_for_python(), scgi_file)
