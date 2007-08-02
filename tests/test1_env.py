#!/usr/bin/env python

from pyscgi import ServerFactory, SCGIHandler

DEFAULT_PORT = 4000
 
class MyHandler(SCGIHandler):
    def __init__ (self, request, client_address, server):
        SCGIHandler.__init__ (self, request, client_address, server)

    def print_env (self):
        self.wfile.write('<table border="0">')
        for k, v in self.env.items():
            self.wfile.write('<tr><td><b>%s</b></td><td>%r</td></tr>' % (k, v))
        self.wfile.write('</table')

    def handle_request (self):
        self.wfile.write('Content-Type: text/html\r\n\r\n')
        self.wfile.write('<h1>Environment variables</h1>')
        self.print_env()

def main():
    srv = ServerFactory(handler_class=MyHandler, port=DEFAULT_PORT)
    srv.serve_forever()

if __name__ == "__main__":
    main()
