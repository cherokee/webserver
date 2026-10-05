from qa_base import *

DIR  = "test_267"
HOST: str = "it_doesnt_exist"
PORT: int = 65056

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name  = "Advised port in redirections"

        g = globals()

        self.request          = f"GET /{g['DIR']} HTTP/1.1\r\n" +\
                                f"Host: {g['HOST']}:{g['PORT']}\r\n" +\
                                "Connection: close\r\n"
        self.expected_error   = 301

    def Prepare (self, www):
        self.Mkdir (www, DIR)

        # is_ssl is only defined after initialisation
        g = globals()
        if self.is_ssl:
            self.expected_content = [f"Location: https://{g['HOST']}:{g['PORT']}/{g['DIR']}/"]
        else:
            self.expected_content = [f"Location: http://{g['HOST']}:{g['PORT']}/{g['DIR']}/"]
