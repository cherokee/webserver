import CTK
import time
import pyscgi

HTTP_HEADER = """\
Status: 200 OK\r\n\
Content-Type: text/html\r\n\
\r\n"""

HEADERS = [
    '<script type="text/javascript" src="/static/js/jquery-1.3.2.js"></script>',
    '<script type="text/javascript" src="/static/js/Submitter.js"></script>'
]

def test3():
    props = CTK.PropsTableAuto ('http://localhost:9091/error')
    props.Add ('Name',    CTK.TextField({'name': "server!uno", 'class':"required"}), 'Example 1')
    props.Add ('Surname', CTK.TextField({'name': "server!dos", 'class':"required"}), 'Lalala')
    props.Add ('Nick',    CTK.TextField({'name': "server!tri", 'class':"optional"}), 'Oh uh ah!')

    page = CTK.Page()
    page.AddHeaders (HEADERS)
    page += props

    return page.Render()

class Handler (pyscgi.SCGIHandler):
    def handle_request (self):
        url = self.env['REQUEST_URI']

        if url == '/ok':
            time.sleep(1)
            content = HTTP_HEADER + "{'ret': 'ok'}"
        elif url == '/error':
            time.sleep(1)
            content = HTTP_HEADER + "{'ret': 'error', 'errors': {'server!dos': 'Bad boy'}}"
        else:
            content = HTTP_HEADER + test3()
        self.send(content)

if __name__ == '__main__':
    srv = pyscgi.ServerFactory (True, handler_class=Handler, host="127.0.0.1", port=8000)

    while True:
        srv.handle_request()
