from base import *

CONF = """
vserver!default!directory!/auth2users!auth = plain
vserver!default!directory!/auth2users!auth!methods = basic
vserver!default!directory!/auth2users!auth!realm = Test with Users
vserver!default!directory!/auth2users!auth!passwdfile = %s
vserver!default!directory!/auth2users!auth!users = foo,Aladdin
vserver!default!directory!/auth2users!priority = 860
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "Auth basic, with users II"
        self.request          = "GET /auth2users/ HTTP/1.0\r\n" +\
                                "Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==\r\n"
        self.expected_error   = 200

    def Prepare (self, www):
        d = self.Mkdir (www, "auth2users")
        self.WriteFile (d, "passwd", 0444, 'user:cherokee\n' + 'foo:bar\n' + 'Aladdin:open sesame\n')

        self.conf = CONF % (d+"/passwd")
        
        self.conf2             = """Directory /auth2users {
                                     Auth Basic {
                                          Name "Test with Users"
                                          Method plain { PasswdFile %s }
                                          User foo, Aladdin
                                     }
                                }""" % (d+"/passwd")
