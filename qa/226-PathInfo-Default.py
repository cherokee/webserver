from base import *

HOST        = "test226"

PATH_INFO   = "/this/is/pathinfo"
SCRIPT_NAME = ""

PORT        = get_free_port()
PYTHON      = look_for_python()

SCRIPT = """
from pyscgi import *

class TestHandler (SCGIHandler):
    def handle_request (self):
        self.send('Content-Type: text/plain\\r\\n\\r\\n')
        self.send('PathInfo is: >'  + self.env['PATH_INFO']   +'<\\n')
        self.send('ScriptName is: >'+ self.env['SCRIPT_NAME'] +'<\\n')

SCGIServerFork(TestHandler, port=%d).serve_forever()
""" % (PORT)

source = get_next_source()

CONF = """
vserver!226!nick = %(HOST)s
vserver!226!document_root = /dev/null

vserver!226!rule!1!match = default
# vserver!226!rule!1!match = directory
# vserver!226!rule!1!match!directory = /
vserver!226!rule!1!handler = scgi
vserver!226!rule!1!handler!check_file = 0
vserver!226!rule!1!handler!balancer = round_robin
vserver!226!rule!1!handler!balancer!source!1 = %(source)d

source!%(source)d!type = interpreter
source!%(source)d!host = localhost:%(PORT)d
source!%(source)d!interpreter = %(PYTHON)s %(scgi_file)s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "PathInfo: default rule"
        self.request          = "GET %s HTTP/1.1\r\n"%(PATH_INFO) +\
                                "Host: %s\r\n"%(HOST) + \
                                "Connection: Close\r\n"
        self.expected_error   = 200
        self.expected_content = ["PathInfo is: >%s<"%(PATH_INFO),
                                 "ScriptName is: >%s<"%(SCRIPT_NAME)]

    def Prepare (self, www):
        scgi_file = self.WriteFile (www, "test226.scgi", 0444, SCRIPT)

        pyscgi = os.path.join (www, 'pyscgi.py')
        if not os.path.exists (pyscgi):
            self.CopyFile ('pyscgi.py', pyscgi)

        vars = globals()
        vars['scgi_file'] = scgi_file
        self.conf = CONF % (vars)
