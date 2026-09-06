import os
from qa_base import *

DIR    = "/FCGI-EmptyVars/"
MAGIC  = "Cherokee and FastCGI rocks!"
PORT   = get_free_port()
PYTHON = look_for_python()

SCRIPT = f"""
from qa_fcgi import *

def app (environ, start_response):
    start_response('200 OK', [("Content-Type", "text/plain")])

    resp = ""
    for k in environ:
          resp += f'{{k}}: {{environ[k]}}\\n'
    return [resp]

WSGIServer(app, bindAddress=("localhost",{PORT})).run()
"""

source = get_next_source()

CONF = """
vserver!1!rule!1410!match = directory
vserver!1!rule!1410!match!directory = %(DIR)s
vserver!1!rule!1410!handler = fcgi
vserver!1!rule!1410!handler!check_file = 0
vserver!1!rule!1410!handler!balancer = round_robin
vserver!1!rule!1410!handler!balancer!source!1 = %(source)d

source!%(source)d!type = interpreter
source!%(source)d!host = localhost:%(PORT)d
source!%(source)d!interpreter = %(PYTHON)s %(fcgi_file)s
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "FastCGI: Variables"

        self.request           = f"GET {DIR} HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = ['PATH_INFO:', 'QUERY_STRING:']
        self.forbidden_content = ['from fcgi', 'start_response']

    def Prepare (self, www):
        fcgi_file = self.WriteFile (www, "fcgi_test_vbles.fcgi", 0o444, SCRIPT)

        fcgi = os.path.join (www, 'fcgi.py')
        if not os.path.exists (fcgi):
            self.CopyFile ('qa_fcgi.py', fcgi)

        vars = globals()
        vars['fcgi_file'] = fcgi_file
        self.conf = CONF % (vars)
