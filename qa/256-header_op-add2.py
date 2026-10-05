from qa_base import *

DIR = "headerop_add2"

HEADERS = [("X-What-Rocks", "Cherokee does"),
           ("X-What-Sucks", "Failed QA tests"),
           ("X-What-Is-It", "A successful test")]

CONF = """
vserver!1!rule!2560!match = directory
vserver!1!rule!2560!match!directory = /%(DIR)s
vserver!1!rule!2560!handler = dirlist
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name           = "Header Ops: Add multiple headers"
        self.request        = f"GET /{DIR}/ HTTP/1.0\r\n"
        self.expected_error = 200
        self.conf           = CONF%(globals())

        n = 2
        for h,v in HEADERS:
            self.conf += f"vserver!1!rule!2560!header_op!{int(n)}!type = add\n"
            self.conf += f"vserver!1!rule!2560!header_op!{int(n)}!header = {h}\n"
            self.conf += f"vserver!1!rule!2560!header_op!{int(n)}!value = {v}\n"
            n += 1

    def CustomTest (self):
        header = self.reply[:self.reply.find("\r\n\r\n")+2]

        for h,v in HEADERS:
            if not f"{h}: {v}\r\n" in header:
                return -1

        return 0

    def Prepare (self, www):
        self.Mkdir (www, DIR)
