#!/usr/bin/env python

from pyscgi import SCGIHandler, ServerFactory

DEFAULT_PORT = 4000
POST_EXAMPLE = """
<form method="post">
  <table border="0">
    <tr><td>Name</td>   <td><input name="name">   </td></tr>
    <tr><td>Surname</td><td><input name="surname"></td></tr>
    <tr><td>Server</td> <td><select name="server">
      <option value="cherokee1">Cherokee stable
      <option value="cherokee2">Cherokee devel
      </server></td></tr>
  </table>
  <input type="submit" value="Submit">
</form>
"""

class MyHandler(SCGIHandler):
    def __init__ (self, request, client_address, server):
        SCGIHandler.__init__ (self, request, client_address, server)

    def handle_request (self):
        self.send('Content-Type: text/html\r\n\r\n')
        self.send('<h1>Post test</h1>')

        self.handle_post()

        if self.post:
            length = len(self.post)
            if length > 0:
                self.send('Post len: %d <br/>'     % (length))
                self.send('Post content: %s <br/>' % (self.post))
                return
        self.send(POST_EXAMPLE)

def main():
    srv = ServerFactory(handler_class=MyHandler, port=DEFAULT_PORT)
    srv.serve_forever()

if __name__ == "__main__":
    main()
