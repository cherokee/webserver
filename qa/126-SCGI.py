import os
from base import *

DIR   = "/SCGI1/"
MAGIC = "Cherokee and SCGI rocks!"
PORT  = 5001

SCRIPT = """
from scgi.scgi_server import *

class TestHandler (SCGIHandler):
    def handle_connection (self, socket):
        s = socket.makefile ('w')
        s.write('Content-Type: text/plain\\r\\n\\r\\n')
        s.write('%s\\n')
        socket.close()
        
SCGIServer(TestHandler, port=%d).serve()
""" % (MAGIC, PORT)


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "SCGI I"

        self.request           = "GET %s HTTP/1.0\r\n" %(DIR)
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["scgi.scgi_server", "SCGIServer", "write"]

    def Prepare (self, www):
        scgi_file = self.WriteFile (www, "scgi_test1.scgi", 0444, SCRIPT)

        self.conf              = """Directory %s {
                                         Handler scgi {
                                            Server localhost:%d {
                                               Interpreter "%s %s"
                                            }
                                         }
                                }""" % (DIR, PORT, PYTHON_PATH, scgi_file)

    def Precondition (self):
        re = os.system ("%s -c 'import scgi.scgi_server'" % (PYTHON_PATH)) 
        return (re == 0)
