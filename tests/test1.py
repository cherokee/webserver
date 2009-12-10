import CTK
import time
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

def test2():
    props = CTK.PropsTable(3)
    props['Name']    = CTK.TextField({'name': "server!uno", 'class':"required"})
    props['Surname'] = CTK.TextField({'name': "server!dos", 'class':"required"})
    props['Nick']    = CTK.TextField({'name': "server!tri", 'class':"optional"})

    submit = CTK.Submitter('http://localhost:9091/error')
    submit += props

    return HTML_PAGE %(submit.Render())

def test3():
    props = CTK.PropsTableAuto (3, 'http://localhost:9091/error')

    props['Name']    = CTK.TextField({'name': "server!uno", 'class':"required"})
    props['Surname'] = CTK.TextField({'name': "server!dos", 'class':"required"})
    props['Nick']    = CTK.TextField({'name': "server!tri", 'class':"optional"})

    return HTML_PAGE %(props.Render())


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
