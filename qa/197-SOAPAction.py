from qa_base import *

DIR       = "soapaction1"
HEADER    = "SOAPAction"
VALUE     = '"urn:wroxheroes"'
FORBIDDEN = "SOAP headers"

CONF = f"""
vserver!1!rule!1970!match = directory
vserver!1!rule!1970!match!directory = /{DIR}
vserver!1!rule!1970!handler = fcgi
vserver!1!rule!1970!handler!balancer = round_robin
vserver!1!rule!1970!handler!balancer!source!1 = 1
vserver!1!rule!1970!handler!pass_req_headers = 1
"""

PHP_SCRIPT = f"""
<?php
   /* {FORBIDDEN} */
   echo "SOAPAction: ".$_SERVER["HTTP_SOAPACTION"];
?>"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "SOAPAction"
        self.request          = f"GET /{DIR}/file.php HTTP/1.0\r\n" + \
                                f"{HEADER}: {VALUE}\r\n"
        self.expected_error   = 200
        self.expected_content = f"{HEADER}: {VALUE}"
        self.conf             = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "file.php", 0o444,
                        '<?php /* {} */ echo "{}: ".$_SERVER["HTTP_SOAPACTION"]; ?>'.format(FORBIDDEN, HEADER))

    def Precondition (self):
        return os.path.exists (look_for_php())
