import os
from base import *

CONF = """
vserver!0660!nick = directoryindex3
vserver!0660!document_root = %s
vserver!0660!directory_index = index.php,/super_test_index.php
vserver!0660!rule!1!match = default
vserver!0660!rule!1!handler = common
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Directory indexer, /index, vbles"

        self.request           = "GET /inside/ HTTP/1.0\r\n" +\
                                 "Host: directoryindex3\r\n"
        self.expected_error    = 200

    def Prepare (self, www):
        self.dr = self.Mkdir (www, "directoryindex3/")
        self.Mkdir (www, "directoryindex3/inside/foo")

        self.expected_content  = ["DocumentRoot %s" % (self.dr),
                                  "ScriptName /super_test_index.php",
                                  "RequestUri /inside/"]

        self.conf = CONF % (self.dr)

        for php in self.php_conf.split("\n"):
            self.conf += "vserver!0660!rule!%s\n" % (php)

    def JustBefore (self, www):
        self.WriteFile (self.dr, "super_test_index.php", 0666, """<?php
                        echo "DocumentRoot ".$_SERVER[DOCUMENT_ROOT]."\n";
                        echo "ScriptName "  .$_SERVER[SCRIPT_NAME]."\n";
                        echo "RequestUri "  .$_SERVER[REQUEST_URI]."\n";
                        ?>""")

    def Precondition (self):
        if not os.path.exists (look_for_php()):
            return False

        f = os.popen("ps -p %d -L | wc -l" % (os.getpid()))
        threads = int(f.read()[:-1])
        f.close()

        return (threads <= 2)
        
