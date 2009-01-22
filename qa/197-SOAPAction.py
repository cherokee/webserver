from base import *

DIR       = "soapaction1"
HEADER    = "SOAPAction"
VALUE     = '"urn:wroxheroes"'
FORBIDDEN = "SOAP headers"

PHP_SCRIPT = """
<?php 
   /* %s */
   echo "SOAPAction: ".$_SERVER["HTTP_SOAPACTION"];
?>""" % (FORBIDDEN)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "SOAPAction"
        self.request          = "GET /%s/test.php HTTP/1.0\r\n" % (DIR)
        self.expected_error   = 200
        self.expected_content = "%s: %s" % (HEADER, VALUE)

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test.php", 0444,
                        '<?php /* %s */ echo "%s: ".$_SERVER["HTTP_SOAPACTION"]; ?>' % 
                        (FORBIDDEN, HEADER))

    def Precondition (self):
        return os.path.exists (look_for_php())

