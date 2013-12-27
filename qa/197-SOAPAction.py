from base import *

DIR       = "soapaction1"
HEADER    = "SOAPAction"
VALUE     = '"urn:wroxheroes"'
FORBIDDEN = "SOAP headers"

CONF = """
vserver!1!rule!1970!match = directory
vserver!1!rule!1970!match!directory = /%s
vserver!1!rule!1970!handler = fcgi
vserver!1!rule!1970!handler!balancer = round_robin
vserver!1!rule!1970!handler!balancer!source!1 = 1
vserver!1!rule!1970!handler!pass_req_headers = 1
""" % (DIR)

PHP_SCRIPT = """
<?php
   /* %s */
   echo "SOAPAction: ".$_SERVER["HTTP_SOAPACTION"];
?>""" % (FORBIDDEN)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "SOAPAction"
        self.request          = "GET /%s/file.php HTTP/1.0\r\n" % (DIR) + \
                                "%s: %s\r\n" % (HEADER, VALUE)
        self.expected_error   = 200
        self.expected_content = "%s: %s" % (HEADER, VALUE)
        self.conf             = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "file.php", 0444,
                        '<?php /* %s */ echo "%s: ".$_SERVER["HTTP_SOAPACTION"]; ?>' %
                        (FORBIDDEN, HEADER))

    def Precondition (self):
        return os.path.exists (look_for_php())

