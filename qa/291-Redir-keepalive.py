from base import *

DIR = "redir-keepalive-1"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Keep-alive after 301 response"
        self.expected_error = 301

        request_base = "GET /%s HTTP/1.1\r\n" % (DIR) + \
                       "Host: example.com\r\n"

        self.request = request_base + \
                       "Connection: keep-alive\r\n" + \
                       "\r\n" + \
                       request_base + \
                       "Connection: close\r\n"

    def Prepare (self, www):
        # Local directory
        self.Mkdir (www, DIR)

    def CustomTest (self):

        parts = self.reply.split ('HTTP/1.1 301')
        if len(parts) != 3:
            return -1

        first_response  = parts[1]
        second_response = parts[2]

        if "close" in first_response.lower():
            return -1

        if not "close" in second_response.lower():
            return -1

        return 0
