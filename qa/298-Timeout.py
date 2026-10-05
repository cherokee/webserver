from qa_base import *

DIR = "298-Timeout"
DIR_RULE = f"{DIR}-rule"
CONTENT = "Tests to check whether timeout is applied."

SERVER_TIMEOUT = 5
RULE_TIMEOUT = 3

CONF = f"""
server!timeout = {int(globals()['SERVER_TIMEOUT'])}
vserver!1!rule!2890!match = directory
vserver!1!rule!2890!match!directory = /{globals()['DIR']}
vserver!1!rule!2890!handler = cgi

vserver!1!rule!2891!match = directory
vserver!1!rule!2891!match!directory = /{globals()['DIR_RULE']}
vserver!1!rule!2891!handler = cgi
vserver!1!rule!2891!timeout = {int(globals()['RULE_TIMEOUT'])}

"""

CGI_CODE = """#!/bin/sh

echo "Content-Type: text/plain"
echo
sleep %(runtime)i
echo "%(content)s"
"""


class TestEntry (TestBase):
    """Test for timeout being applied.

    If timeout expires, no content after `sleep` in the CGI will
    be delivered.
    """

    def __init__ (self, dir, filename, runtime, content, expected_timeout):
        TestBase.__init__ (self, __file__)
        self.request = f"GET /{dir}/{filename} HTTP/1.0\r\n" +\
                       "Connection: close\r\n"
        self.expected_error = 200

        if runtime < expected_timeout:
            self.expected_content = content
        else:
            self.forbidden_content = content


class Test (TestCollection):

    def __init__ (self):
        TestCollection.__init__ (self, __file__)

        self.name = "Connection Timeouts Applied"
        self.conf = CONF
        self.proxy_suitable = True
        self.filenames = {DIR: [],
                          DIR_RULE: []}

    def Prepare (self, www):
        self.local_dirs = {DIR: self.Mkdir (www, DIR),
                           DIR_RULE: self.Mkdir (www, DIR_RULE)}

    def JustBefore (self, www):
        # Create sub-request objects
        self.Empty ()

        # Create all tests with different runtime lengths
        # Instant return and 1 second less than timeout should work,
        # but past the timeout should return no content.
        for dir, timeout in ((DIR, SERVER_TIMEOUT), (DIR_RULE, RULE_TIMEOUT)):
            for script_runtime in (0, timeout-2, timeout+2):
                # Write the new script files
                filename = f'test-{int(script_runtime)}-seconds.cgi'
                code = CGI_CODE % dict(runtime=script_runtime, content=CONTENT)
                self.WriteFile (self.local_dirs[dir], filename, 0o755, code)
                self.filenames[dir].append(filename)

                obj = self.Add (TestEntry (dir,
                                           filename,
                                           runtime=script_runtime,
                                           content=CONTENT,
                                           expected_timeout=timeout))


    def JustAfter (self, www):
        # Clean up the local files
        for dir in self.local_dirs:
            for filename in self.filenames[dir]:
                fp = os.path.join (self.local_dirs[dir], filename)
                os.unlink (fp)
        self.filenames = {}
