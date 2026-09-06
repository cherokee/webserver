from qa_base import *

DIR    = 'foo/bar'
FILE   = 'file.cgi'
MAGIC  = 'Alvaro: http://www.alobbs.com/'
DOMAIN = 'domain_207'

CONF = """
vserver!207!nick = %(DOMAIN)s
vserver!207!document_root = %(droot)s

vserver!207!rule!2!match = exists
vserver!207!rule!2!match!exists = %(FILE)s
vserver!207!rule!2!match!final = 1
vserver!207!rule!2!handler = cgi

vserver!207!rule!1!match = default
vserver!207!rule!1!handler = file
"""

CGI_BASE = """#!/bin/sh
echo "Content-type: text/html"
echo ""
cat << EOF
%s
EOF
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Rule Exists: match in directory"
        self.request           = f"GET /{DIR}/{FILE} HTTP/1.0\r\n" + \
                                 f"Host: {DOMAIN}\r\n"
        self.forbidden_content = ['/bin/sh', 'echo']
        self.expected_error    = 200
        self.expected_content  = MAGIC

    def Prepare (self, www):
        droot = self.Mkdir (www, '207')
        e     = self.Mkdir (droot, DIR)

        self.WriteFile (e, FILE, 0o555, CGI_BASE % (MAGIC))

        vars = globals()
        vars['droot'] = droot
        self.conf     = CONF % (vars)
