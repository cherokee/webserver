#!/usr/bin/env python

from pyscgi import SCGIHandler, SCGIServer

DEFAULT_PORT = 4000
 
class MyHandler(SCGIHandler):
    def __init__ (self, request, client_address, server):
        SCGIHandler.__init__ (self, request, client_address, server)

    def print_env (self):
        self.output.write('<table border="0">')
        for k, v in self.env.items():
            self.output.write('<tr><td><b>%s</b></td><td>%r</td></tr>' % (k, v))
        self.output.write('</table')

    def handle_request (self):
        self.output.write('Content-Type: text/html\r\n\r\n')
        self.output.write('<h1>Environment variables</h1>')
        self.print_env()

def main():
    srv = SCGIServer(handler_class=MyHandler, port=DEFAULT_PORT)
    srv.serve_forever()

if __name__ == "__main__":
    main()
