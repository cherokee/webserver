from base import *

DIR  = "test_267"
HOST = "it_doesnt_exist"
PORT = 65056

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name  = "Advised port in redirections"

        g = globals()

        self.request          = "GET /%(DIR)s HTTP/1.1\r\n"   %(g) +\
                                "Host: %(HOST)s:%(PORT)s\r\n" %(g) +\
                                "Connection: close\r\n"
        self.expected_error   = 301

    def Prepare (self, www):
        self.Mkdir (www, DIR)

        # is_ssl is only defined after initialisation
        g = globals()
        if self.is_ssl:
            self.expected_content = ['Location: https://%(HOST)s:%(PORT)s/%(DIR)s/'%(g)]
        else:
            self.expected_content = ['Location: http://%(HOST)s:%(PORT)s/%(DIR)s/'%(g)]
