import os
from base import *

DIR = "/299-MethodsRequestBodyHandling/"
MAGIC = "Report bugs to http://bugs.cherokee-project.com"
PORT   = get_free_port()
PYTHON = look_for_python()
SOURCE = get_next_source()

METHODS = {
    'required': [
        'POST',
        'PUT',
        'MERGE',
        'SEARCH',
        'REPORT',
        'PATCH',
        'PROPFIND',
        'PROPPATCH',
        'UPDATE',
        'LABEL',
    ],
    'optional': [
        'OPTIONS',
        'DELETE',
        'MKCOL',
        'COPY',
        'MOVE',
        'LOCK',
        'UNLOCK',
        'VERSION-CONTROL',
        'CHECKOUT',
        'UNCHECKOUT',
        'CHECKIN',
        'MKWORKSPACE',
        'MKACTIVITY',
        'BASELINE-CONTROL',
    ]
}

CONF = """
vserver!1!rule!2990!match = directory
vserver!1!rule!2990!match!directory = %(DIR)s
vserver!1!rule!2990!handler = fcgi
vserver!1!rule!2990!handler!check_file = 0
vserver!1!rule!2990!handler!balancer = round_robin
vserver!1!rule!2990!handler!balancer!source!1 = %(SOURCE)d

source!%(SOURCE)d!type = interpreter
source!%(SOURCE)d!host = localhost:%(PORT)d
source!%(SOURCE)d!interpreter = %(PYTHON)s %(fcgi_file)s
"""

SCRIPT = """
from fcgi import *

def app (environ, start_response):
    start_response('200 OK', [("Content-Type", "text/plain")])
    
    response = "Method: %%s\\n" %% environ['REQUEST_METHOD']
    if 'CONTENT_LENGTH' in environ:
        request_body_size = int(environ.get('CONTENT_LENGTH', 0))
        request_body = environ['wsgi.input'].read(request_body_size)
        response += "Body: %%s\\n" %% request_body

    return [response]

WSGIServer(app, bindAddress=("localhost",%d)).run()
""" % (PORT)


class TestEntry (TestBase):
    """Test for HTTP methods with required and optional request bodies being
    received and processed correctly by Cherokee.
    """

    def __init__ (self, method, send_input, input_required):
        TestBase.__init__ (self, __file__)
        self.request = "%s %s HTTP/1.0\r\n" % (method, DIR) +\
                       "Content-type: text/xml\r\n"
        self.expected_content = []
        
        if input_required and not send_input:
            self.expected_error = 411
        else:
            self.expected_error = 200
            self.expected_content.append("Method: %s" % method)

        if send_input:
            self.request += "Content-length: %d\r\n" % (len(MAGIC))
            self.post = MAGIC
            self.expected_content.append("Body: %s" % MAGIC)
        else:
            self.forbidden_content = 'Body:'


class Test (TestCollection):

    def __init__ (self):
        TestCollection.__init__ (self, __file__)
        self.name = "Method Request Body Handling"

    def Prepare (self, www):
        fcgi_file = self.WriteFile (www, "fcgi_test_methods.cgi", 0444, SCRIPT)
        
        fcgi = os.path.join (www, 'fcgi.py')
        if not os.path.exists (fcgi):
            self.CopyFile ('fcgi.py', fcgi)

        vars = globals()
        vars['fcgi_file'] = fcgi_file
        self.conf = CONF % (vars)

    def JustBefore (self, www):
        # Create sub-request objects
        self.Empty ()

        # Create all tests with different methods - all methods
        # should only work with request bodies. With no request
        # body a 411 Length Required should result.
        for method in METHODS['required']:
            self.Add (TestEntry (method=method,
                                 send_input=True,
                                 input_required=True))
            self.Add (TestEntry (method=method,
                                 send_input=False,
                                 input_required=True))

        # Create all tests with different methods - all methods
        # should work with and without request bodies.
        for method in METHODS['optional']:
            #Test method when for when sending a request body
            self.Add (TestEntry (method=method,
                                 send_input=True,
                                 input_required=False))
            #Test method when not sending a request body
            self.Add (TestEntry (method=method,
                                 send_input=False,
                                 input_required=False))
