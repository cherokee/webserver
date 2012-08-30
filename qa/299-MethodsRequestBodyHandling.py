from base import *

DIR = "299-MethodsRequestBodyHandling"
MAGIC = "Report bugs to http://bugs.cherokee-project.com"

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
vserver!1!rule!1280!match = directory
vserver!1!rule!1280!match!directory = /%s
vserver!1!rule!1280!handler = common
""" % (DIR)


class TestEntry (TestBase):
    """Test for HTTP methods with required and optional request bodies being
    received and processed correctly by Cherokee.
    """

    def __init__ (self, method, send_input, input_required):
        TestBase.__init__ (self, __file__)
        self.request = "%s /%s/ HTTP/1.0\r\n" % (method, DIR) +\
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
        self.conf = CONF
        self.proxy_suitable = True

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, "test_index.php", 0444,
                            """
<?php echo 'Method: '.$_SERVER['REQUEST_METHOD']; ?>

<?php
    $body = @file_get_contents('php://input');
    if (strlen($body) > 0):
        echo "Body: $body";
    endif;
?>
                            """)

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
