import os
from base import *

DIR   = "/FCGI-EmptyVars/"
MAGIC = "Cherokee and FastCGI rocks!"
PORT  = get_free_port()

SCRIPT = """
from fcgi import *

def app (environ, start_response):
    start_response('200 OK', [("Content-Type", "text/plain")])

    resp = ""
    for k in environ:
          resp += '%%s: %%s\\n' %% (k, environ[k])
    return [resp]
 
WSGIServer(app, bindAddress=("localhost",%d)).run()
""" % (PORT)

CONF = """
vserver!default!directory!<dir>!handler = fcgi
vserver!default!directory!<dir>!handler!balancer = round_robin
vserver!default!directory!<dir>!handler!balancer!type = interpreter
vserver!default!directory!<dir>!handler!balancer!1!host = localhost:%d
vserver!default!directory!<dir>!handler!balancer!1!interpreter = %s %s
vserver!default!directory!<dir>!priority = 1410
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "FastCGI: Variables"

        self.request           = "GET %s HTTP/1.0\r\n" %(DIR) 
        self.expected_error    = 200
        self.expected_content  = ['PATH_INFO:', 'QUERY_STRING:']
        self.forbidden_content = ['from fcgi', 'start_response']

    def Prepare (self, www):
        fcgi_file = self.WriteFile (www, "fcgi_test_vbles.fcgi", 0444, SCRIPT)

        fcgi = os.path.join (www, 'fcgi.py')
        if not os.path.exists (fcgi):
            self.CopyFile ('fcgi.py', fcgi)

        self.conf = CONF % (PORT, look_for_python(), fcgi_file)
        self.conf = self.conf.replace ('<dir>', DIR)
