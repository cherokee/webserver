from base import *

NICK1  = "nick-2720"
ERROR1 = 503

NICK2  = "nick-2720"
ERROR2 = 502

CONF = """
# This Vserver is not matched because of "!match!nick = 0".
#
vserver!2721!nick = %(NICK2)s
vserver!2721!document_root = /dev/null

vserver!2721!match!nick = 0
vserver!2721!match = wildcard
vserver!2721!match!domain!1 = foobar

vserver!2721!rule!1!match = default
vserver!2721!rule!1!handler = custom_error
vserver!2721!rule!1!handler!error = %(ERROR2)s

# This one has the same nick. It's matched.
#
vserver!2720!nick = %(NICK1)s

vserver!2720!match!nick = 0
vserver!2720!match = wildcard
vserver!2720!match!domain!1 = foobar2

vserver!2720!document_root = /dev/null
vserver!2720!rule!1!match = default
vserver!2720!rule!1!handler = custom_error
vserver!2720!rule!1!handler!error = %(ERROR1)s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name           = "VServer nick match"
        self.request        = "GET / HTTP/1.0\r\n" + \
                              "Host: %s\r\n" %(NICK2)
        self.expected_error = ERROR2
        self.conf           = CONF %(globals())
