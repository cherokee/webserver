from base import *

ERROR     = 403
ERROR_MSG = """<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<head>
 <title>Permission Denied - Cherokee Web Server</title>
</head>

 <body>

 <!-- Poem by Thomas Thurman <thomas@thurman.org.uk> -->

 <h1>403 Access Denied</h1>

<p>So many years have passed since first you sought
the lands beyond the edges of the sky,
so many moons reflected in your eye,
(familiar newness, fear of leaving port),
since first you sought, and failed, and learned to fall,
(first hope, then cynicism, silent dread,
the countless stars, still counting overhead
the seconds to your final voyage of all...)
   and last, in glory gold and red around
   your greatest search, your final quest to know!
   yet... ashes drift, the embers cease to glow,
   and darkened life in frozen death is drowned;
and ashes on the swell are seen no more.
The silence surges.

<p><b>Error 403</b>.
 </body>
</html>"""

CONF = """
vserver!1!rule!1120!match = directory
vserver!1!rule!1120!match!directory = /cgi_error_403_1
vserver!1!rule!1120!handler = cgi
vserver!1!rule!1120!handler!error_handler = 1
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "CGI error message"

        self.request           = "GET /cgi_error_403_1/exec.cgi HTTP/1.0\r\n"
        self.expected_error    = ERROR
        self.expected_content  = ERROR_MSG
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, "cgi_error_403_1")
        f = self.WriteFile (d, "exec.cgi", 0555,
                            """#!/bin/sh

                            echo "Content-type: text/html"
                            echo "Status: %s"
                            echo ""
                            cat << EOF
                            %s
                            EOF
                            """ % (ERROR, ERROR_MSG))
