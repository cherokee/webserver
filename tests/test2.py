import CTK
import time
import pyscgi

HTTP_HEADER = """\
Status: 200 OK\r\n\
Content-Type: text/html\r\n\
\r\n"""

def test():
    temp = CTK.Template('test2.html')

    header = "<!-- Empty -->"
    body   = "%(first)s part, and %(second)s part."
    first  = "1st"
    second = "2nd"

    return temp.Render()

class Handler (pyscgi.SCGIHandler):
    def handle_request (self):
        url = self.env['REQUEST_URI']

        content = HTTP_HEADER + test()
        self.send(content)

if __name__ == '__main__':
    srv = pyscgi.ServerFactory (True, handler_class=Handler, host="127.0.0.1", port=8000)

    while True:
        srv.handle_request()
