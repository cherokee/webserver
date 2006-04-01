from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Compaq Web Enterprise Management bug"

        self.request           = "GET /<!.StringHttpRequest=Url> HTTP/1.1\r\n"        + \
                                 "Connection: Close\r\n"                              + \
                                 "Host: localhost\r\n"                                + \
                                 "Pragma: no-cache\r\n"                               + \
                                 "User-Agent: Mozilla/4.75 [en] (X11, U; Nessus)\r\n" + \
                                 "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, image/png, */*\r\n" + \
                                 "Accept-Language: en\r\n"                            + \
                                 "Accept-Charset: iso-8859-1,*,utf-8\r\n"
                                 
        self.expected_error    = 404


