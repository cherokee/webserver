from base import *

NICK      = "nick-2730"
ERROR_MSG = "Error handler: it worked fine."

CONF = """
vserver!2730!nick = %(NICK)s
vserver!2730!document_root = %(droot)s

vserver!2730!error_handler = error_redir
vserver!2730!error_handler!default!show = 0
vserver!2730!error_handler!default!url = /errors/default.txt

vserver!2730!rule!1!match = default
vserver!2730!rule!1!handler = file
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "Unknown method error redirection"
        self.request          = "GIMME / HTTP/1.0\r\n" + \
                                "Host: %s\r\n" %(NICK)
        self.expected_error   = 405 # Method Not Allowed
        self.expected_content = ERROR_MSG

    def Prepare (self, www):
        droot = self.Mkdir (www, "%s_droot"%(NICK))
        e = self.Mkdir (droot, "errors")
        self.WriteFile (e, "default.txt", 0444, ERROR_MSG)

        vars = globals()
        vars.update(locals())
        self.conf = CONF %(vars)
