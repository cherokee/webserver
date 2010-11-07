from base import *

DOMAIN         = "test261"
ERROR_NUM      = 504
DIR_STATIC     = "errors"
FILE_DEFAULT   = "error-default.txt"
FILE_3xx       = "error-3xx.txt"
ERROR_DEFAULT  = "This is the default error"
ERROR_3xx      = "This is a 3xx error"

CONF = """
vserver!261!nick = %(DOMAIN)s
vserver!261!document_root = %(droot)s

vserver!261!rule!1!match = default
vserver!261!rule!1!handler = custom_error
vserver!261!rule!1!handler!error = %(ERROR_NUM)s

vserver!261!rule!2!match = directory
vserver!261!rule!2!match!directory = /%(DIR_STATIC)s
vserver!261!rule!2!handler = file

vserver!261!error_handler = error_redir
vserver!261!error_handler!300!show = 0
vserver!261!error_handler!300!url = /%(DIR_STATIC)s/%(FILE_3xx)s
vserver!261!error_handler!301!show = 0
vserver!261!error_handler!301!url = /%(DIR_STATIC)s/%(FILE_3xx)s
vserver!261!error_handler!default!show = 0
vserver!261!error_handler!default!url = /%(DIR_STATIC)s/%(FILE_DEFAULT)s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Error handler: Redir, default error"

        self.request           = "GET /doesnt-exist HTTP/1.0\r\n" + \
                                 "Host: %s\r\n" % (DOMAIN)
        self.expected_error    = ERROR_NUM
        self.forbidden_content = ERROR_3xx
        self.expected_content  = ERROR_DEFAULT

    def Prepare (self, www):
        droot  = self.Mkdir (www, DOMAIN)
        static = self.Mkdir (droot, DIR_STATIC)

        rep = globals()
        rep['droot'] = droot
        self.conf = CONF %(rep)

        self.WriteFile (static, FILE_3xx,     0644, ERROR_3xx)
        self.WriteFile (static, FILE_DEFAULT, 0644, ERROR_DEFAULT)
