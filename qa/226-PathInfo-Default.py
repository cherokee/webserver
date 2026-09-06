from qa_base import *

HOST: str = "test226"

PATH_INFO   = "/this/is/pathinfo"
SCRIPT_NAME = ""

PORT        = get_free_port()
PYTHON      = look_for_python()

SCRIPT = f"""
from pyscgi import *

class TestHandler (SCGIHandler):
    def handle_request (self):
        self.send('Content-Type: text/plain\\r\
\\r\
')
        self.send('PathInfo is: >'  + self.env['PATH_INFO']   +'<\
')
        self.send('ScriptName is: >'+ self.env['SCRIPT_NAME'] +'<\
')

SCGIServerFork(TestHandler, port={int(PORT)}).serve_forever()
"""

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
        self.request          = f"GET {PATH_INFO} HTTP/1.1\r\n" +\
                                f"Host: {HOST}\r\n" + \
                                "Connection: Close\r\n"
        self.expected_error   = 200
        self.expected_content = [f"PathInfo is: >{PATH_INFO}<",
                                 f"ScriptName is: >{SCRIPT_NAME}<"]

    def Prepare (self, www):
        scgi_file = self.WriteFile (www, "test226.scgi", 0o444, SCRIPT)

        pyscgi = os.path.join (www, 'pyscgi.py')
        if not os.path.exists (pyscgi):
            self.CopyFile ('pyscgi.py', pyscgi)

        vars = globals()
        vars['scgi_file'] = scgi_file
        self.conf = CONF % (vars)
