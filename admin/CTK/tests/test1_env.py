#!/usr/bin/env python

from CTK.pyscgi import ServerFactory, SCGIHandler

DEFAULT_PORT = 4000
 
class MyHandler(SCGIHandler):
    def __init__ (self, request, client_address, server):
        SCGIHandler.__init__ (self, request, client_address, server)

    def print_env (self):
        self.send('<table border="0">')
        for k, v in list(self.env.items()):
            self.send(f'<tr><td><b>{k}</b></td><td>{v!r}</td></tr>')
        self.send('</table')

    def handle_request (self):
        self.send('Content-Type: text/html\r\n\r\n')
        self.send('<h1>Environment variables</h1>')
        self.print_env()

def main():
    srv = ServerFactory(handler_class=MyHandler, port=DEFAULT_PORT)
    srv.serve_forever()

if __name__ == "__main__":
    main()
