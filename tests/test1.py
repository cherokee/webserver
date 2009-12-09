import CTK
import pyscgi

HTML_PAGE = """\
<html><head>
<script type="text/javascript" src="/static/js/jquery-1.3.2.js"></script>
<script type="text/javascript" src="/static/js/Submitter.js"></script>
</head><body>
%s
</body></html>"""

HTTP_HEADER = """\
Status: 200 OK\r\n\
Content-Type: text/html\r\n\
\r\n"""

def test():
    entry1 = CTK.TextField({'name': "server!uno", 'class':"required", 'value':"algo"})
    entry2 = CTK.TextField({'name': "server!dos", 'class':"optional"})
    entry3 = CTK.TextField({'name': "server!tri", 'class':"required"})

    submit = CTK.Submitter('http://localhost:9091/error')

    submit += entry1
    submit += entry2
    submit += entry3

    return HTML_PAGE %(submit.Render())

class Handler (pyscgi.SCGIHandler):
    def handle_request (self):
        url = self.env['REQUEST_URI']
        
        if url == '/ok':
            content = HTTP_HEADER + "{'ret': 'ok'}"
        elif url == '/error':
            content = HTTP_HEADER + "{'ret': 'error', 'errors': {'server!dos': 'Bad boy'}}"
        else:
            content = HTTP_HEADER + test()
        self.send(content)

if __name__ == '__main__':
    srv = pyscgi.ServerFactory (True, handler_class=Handler, host="127.0.0.1", port=8000)

    while True:
        srv.handle_request()
