from base import *

USER_AGENT        = "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.0.1) Gecko/20060313 Debian/1.5.dfsg+1.5.0.1-4 Firefox/1.5.0.1"
ACCEPT            = "text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5"
ACCEPT_LANGUAGE   = "en-us,en;q=0.5"
ACCEPT_ENCODING   = "whatever"
ACCEPT_CHARSET    = "ISO-8859-1,utf-8;q=0.7,*;q=0.7"
CONNECTION        = "Keep-Alive"
KEEP_ALIVE        = "300"
REFERER           = "http://www.cherokee-project.com"
COOKIE            = "Cherokee_cookie"
IF_NONE_MATCH     = "Fancy header"
IF_RANGE          = "Second fancy header"
IF_MODIFIED_SINCE = "Sun Mar 26 16:47:40 IST 2006"


HEADER_VAL="Cherokee_supports_If-None-Match"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "PHP: HTTP headers"
        self.request          = "GET /http_headers1/test.php HTTP/1.0\r\n" + \
                                "User-Agent: %s\r\n"        % (USER_AGENT) + \
                                "Accept: %s\r\n"            % (ACCEPT) + \
                                "Accept-Language: %s\r\n"   % (ACCEPT_LANGUAGE) + \
                                "Accept-Encoding: %s\r\n"   % (ACCEPT_ENCODING) + \
                                "Accept-Charset: %s\r\n"    % (ACCEPT_CHARSET) + \
                                "Connection: %s\r\n"        % (CONNECTION) + \
                                "Keep-Alive: %s\r\n"        % (KEEP_ALIVE) + \
                                "Referer: %s\r\n"           % (REFERER) + \
                                "Cookie: %s\r\n"            % (COOKIE) + \
                                "If-None-Match: %s\r\n"     % (IF_NONE_MATCH) + \
                                "If-Range: %s\r\n"          % (IF_RANGE) + \
                                "If-Modified-Since: %s\r\n" % (IF_MODIFIED_SINCE)

        self.expected_error   = 200
        self.expected_content = ["User-Agent - "        + USER_AGENT,
                                 "Accept - "            + ACCEPT,
                                 "Accept-Language - "   + ACCEPT_LANGUAGE,
                                 "Accept-Charset - "    + ACCEPT_CHARSET,
                                 "Connection - "        + CONNECTION,
                                 "Keep-Alive - "        + KEEP_ALIVE,
                                 "Referer - "           + REFERER,
                                 "Cookie - "            + COOKIE,
                                 "If-None-Match - "     + IF_NONE_MATCH,
                                 "If-Range - "          + IF_RANGE,
                                 "If-Modified-Since - " + IF_MODIFIED_SINCE]

    def Prepare (self, www):
        d = self.Mkdir (www, "http_headers1")
        self.WriteFile (d, "test.php", 0444, """<?php
           echo 'User-Agent - '        . $_SERVER['HTTP_USER_AGENT']        .'\n';
           echo 'Accept - '            . $_SERVER['HTTP_ACCEPT']            .'\n';
           echo 'Accept-Language - '   . $_SERVER['HTTP_ACCEPT_LANGUAGE']   .'\n';
           echo 'Accept-Charset - '    . $_SERVER['HTTP_ACCEPT_CHARSET']    .'\n';
           echo 'Connection - '        . $_SERVER['HTTP_CONNECTION']        .'\n';
           echo 'Keep-Alive - '        . $_SERVER['HTTP_KEEP_ALIVE']        .'\n';
           echo 'Referer - '           . $_SERVER['HTTP_REFERER']           .'\n';
           echo 'Cookie - '            . $_SERVER['HTTP_COOKIE']            .'\n';
           echo 'If-None-Match - '     . $_SERVER['HTTP_IF_NONE_MATCH']     .'\n';
           echo 'If-Range - '          . $_SERVER['HTTP_IF_RANGE']          .'\n';
           echo 'If-Modified-Since - ' . $_SERVER['HTTP_IF_MODIFIED_SINCE'] .'\n';
        ?>""")

    def Precondition (self):
        return os.path.exists (look_for_php())
