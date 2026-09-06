# -*- coding: utf-8 -*-

# Cherokee QA Tests
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
# This file is distributed under the GPL license.

import os
import importlib.util
import sys
import time
import types
import socket
import string
import tempfile
import errno
import ssl

from qa_conf import *
from qa_util import *

DEFAULT_READ = 8192

def importfile(path):
    filename = os.path.basename(path)
    name, ext = os.path.splitext(filename)

    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise ImportError(f"Could not load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)

    return module


class TestBase:
    def __init__ (self, file):
        self.file                    = file    # 001-basic.py
        self.name                    = None    # Test 01: Basic functionality
        self.conf                    = None    # Directory /test { .. }
        self.request                 = ""      # GET / HTTP/1.0
        self.proxy_suitable          = True
        self.post                    = None
        self.expected_error          = None
        self.expected_content        = None
        self.forbidden_content       = None
        self.disabled                = False
        self.tmp: str = ""
        self.nobody: bool = False
        self.php_conf: str | None = None
        self.digest: TestBase.Digest | None = None
        self.local_dir: str = ""
        self.filename: str | None = None
        self._initialize()

    def _initialize (self):
        self.ssl                     = None
        self.is_ssl                  = False
        self.reply                   = ""      # "200 OK"..
        self.version                 = None    # HTTP/x.y: 9, 0 or 1
        self.reply_err               = None    # 200

    def _safe_read (self, s):
         while True:
            try:
                if self.ssl:
                    return self.ssl.recv (DEFAULT_READ).decode("latin-1")
                return s.recv (DEFAULT_READ).decode("latin-1")
            except socket.error as xxx_todo_changeme:
                (err, strerr) = xxx_todo_changeme.args
                if err == errno.EAGAIN or \
                   err == errno.EWOULDBLOCK or \
                   err == errno.EINPROGRESS:
                    continue
            raise

    def _do_request (self, host, port):
        for res in socket.getaddrinfo (host, port, socket.AF_UNSPEC, socket.SOCK_STREAM):
            af, socktype, proto, canonname, sa = res

            try:
                s = socket.socket(af, socktype, proto)
            except socket.error as msg:
                continue

            try:
                s.connect(sa)
            except socket.error as msg:
                s.close()
                s = None
                continue
            break

        if s is None:
            raise Exception("Couldn't connect to the server")

        if self.is_ssl:
            try:
                self.ssl = ssl.create_default_context().wrap_socket(s, server_hostname=host)
            except Exception as e:
                raise Exception("Couldn't handshake SSL: "+str(e))

        request = self.request + "\r\n"
        if self.post is not None:
            request += self.post

        if self.ssl:
            n = self.ssl.send (request.encode())
        else:
            n = s.send (request.encode())
        assert n == len(request)

        while True:
            try:
                d = self._safe_read (s)
            except Exception as e:
                d = ''

            if not len(d):
                break
            self.reply += d

        s.close()

    def _parse_output (self):
        if not len(self.reply):
            raise Exception("Empty header")

        # Protocol version
        reply = self.reply.split('\r', 1)[0]
        if reply[:8] == "HTTP/0.9":
            self.version = 9
        elif reply[:8] == "HTTP/1.0":
            self.version = 0
        elif reply[:8] == "HTTP/1.1":
            self.version = 1
        else:
            raise Exception(f"Invalid header, len={len(reply)}: '{reply}'")

        # Error code
        reply = reply[9:]
        try:
            self.reply_err = int (reply[:3])
        except:
            raise Exception(f"Invalid header, version={int(self.version)} len={len(reply)}: '{reply}'")

    def _check_result_expected_item (self, item):
        if item.startswith("file:"):
            f = open (item[5:])
            error = not f.read() in self.reply
            f.close
            if error:
                return -1
        else:
            if not item in self.reply:
                return -1

    def _check_result_forbidden_item (self, item):
        if item.startswith("file:"):
            f = open (item[5:])
            error = f.read() in self.reply
            f.close
            if error:
                return -1
        else:
            if item in self.reply:
                return -1

    def _check_result (self):
        if self.reply_err != self.expected_error:
            return -1

        if self.expected_content != None:
            if type(self.expected_content) in (bytes, str):
                r = self._check_result_expected_item (self.expected_content)
                if r == -1:
                    return -1
            elif type(self.expected_content) == list:
                for entry in self.expected_content:
                    r = self._check_result_expected_item (entry)
                    if r == -1:
                        return -1
            else:
                raise Exception("Syntax error")


        if self.forbidden_content != None:
            if type(self.forbidden_content) in (bytes, str):
                r = self._check_result_forbidden_item (self.forbidden_content)
                if r == -1:
                    return -1
            elif type(self.forbidden_content) == list:
                for entry in self.forbidden_content:
                    r = self._check_result_forbidden_item (entry)
                    if r == -1:
                        return -1
            else:
                raise Exception("Syntax error")

        r = self.CustomTest()
        if r == -1:
            return -1

        return 0

    def Clean (self):
        self._initialize()

    def Precondition (self):
        return True

    def Prepare (self, www):
        None

    def JustBefore (self, www):
        None

    def JustAfter (self, www):
        None

    def CustomTest (self):
        return 0

    def Run (self, host, port):
        self._do_request(host, port)
        self._parse_output()
        return self._check_result()

    def __str__ (self):
        # Figure the test number
        tmp = self.file.split("-")
        if len(tmp) and tmp[0].isdigit():
            num = tmp[0]
        else:
            num = "???"

        # Render the string
        src  = f"\tName     = {self.name}\n"
        src += f"\tNumber   = {num}\n"

        if self.version == 9:
            src += "\tProtocol = HTTP/0.9\n"
        elif self.version == 0:
            src += "\tProtocol = HTTP/1.0\n"
        elif self.version == 1:
            src += "\tProtocol = HTTP/1.1\n"

        if self.conf is not None:
            src += f"\tConfig   = {self.conf}\n"

        header_full = self.reply.split("\r\n\r\n")[0]
        headers     = header_full.split("\r\n")
        requests    = self.request.split("\r\n")

        src += f"\tRequest  = {requests[0]}\n"
        for request in requests[1:]:
            if len(request) > 1:
                src += f"\t\t{request}\n"

        if self.post is not None and not self.nobody:
            src += f"\tPost     = {self.post}\n"

        if self.expected_error is not None:
            src += f"\tExpected = Code: {int(self.expected_error)}\n"
        else:
            src += "\tExpected = Code: UNSET!\n"

        if self.expected_content is not None:
            src += f"\tExpected = Content: {self.expected_content}\n"

        if self.forbidden_content is not None:
            src += f"\tForbidden= Content: {self.forbidden_content}\n"

        src += f"\tReply    = {headers[0]}\n"
        for header in headers[1:]:
            src += f"\t\t{header}\n"

        if not self.nobody:
            body = self.reply[len(header_full)+4:]
            src += f"\tBody len = {len(body)}\n"
            src += f"\tBody     = {body}\n"

        return src

    def Mkdir (self, www, dir, mode=0o777):
        fulldir = os.path.join (www, dir)
        os.makedirs(fulldir, mode)
        return fulldir

    def WriteFile (self, www, filename, mode=0o444, content=''):
        assert(type(mode) == int)

        fullpath = os.path.join (www, filename)
        f = open (fullpath, 'w')
        f.write (content)
        f.close()
        os.chmod(fullpath, mode)
        return fullpath

    def SymLink (self, source, target):
        os.symlink (source, target)

    def CopyFile (self, src, dst):
        open (dst, 'w').write (open (src, 'r').read())

    def Remove (self, www, filename):
        fullpath = os.path.join (www, filename)
        if os.path.isfile(fullpath):
            os.unlink (fullpath)
        else:
            from shutil import rmtree
            try:
                rmtree (fullpath)
            except: pass

    def WriteTemp (self, content):
        while True:
            name = self.tmp + f"/{letters_random(40)}"
            if not os.path.exists(name): break

        f = open (name, "w+")
        f.write (content)
        f.close()
        return name

    class Digest:
        def __init__ (self):
            self.response = None
            self.vals     = {}

        def ParseHeader (self, reply):
            ret = {"cnonce":"",
                   "nonce":"",
                   "qop":"",
                   "nc":""}

            pos1 = reply.find ("WWW-Authenticate: Digest ") + 25
            pos2 = reply.find ("\r", pos1)
            line = reply[pos1:pos2]

            for item in line.split(", "):
                pos   = item.find("=")
                name  = item[:pos]
                value = item[pos+1:]

                if value[0] == '"':
                    value = value[1:]
                if value[-1] == '"':
                    value = value[:-1]

                ret[name] = value
            return ret

        def CalculateResponse (self, user, realm, passwd, method, url, nonce, qop, cnonce, nc):
            from hashlib import md5

            md5obj = md5()
            md5obj.update(f"{user}:{realm}:{passwd}".encode())
            a1 = md5obj.hexdigest()

            md5obj = md5()
            md5obj.update(f"{method}:{url}".encode())
            ha2 = md5obj.hexdigest()

            md5obj = md5()
            md5obj.update(f"{a1}:{nonce}:".encode())

            if qop != None:
                md5obj.update(f"{nc}:".encode())
                md5obj.update(f"{cnonce}:".encode())
                md5obj.update(f"{qop}:".encode())

            md5obj.update(ha2.encode())
            final = md5obj.hexdigest()

            return final

    def Precondition_UserHome (self, home_subdir="public_html"):
        try:
            user = os.getlogin()
        except OSError:
            return False

        home = os.path.expanduser("~")
        if not home:
            return False

        public_html = os.path.join (home, home_subdir)

        # Look for the public_html directory
        if not os.path.exists(public_html):
            return False

        return public_html, user


class TestCollection (TestBase):
    def __init__ (self, file):
        TestBase.__init__(self, file)

        self.tests    = []
        self.num      = 0
        self.disabled = False
        self.delay    = 0

    def Add (self, test):
        self.num += 1

        if (test.name == None) or len(test.name) == 0:
            test.name = (self.name or "Test") + f", Part {int(self.num)}"

        test.tmp            = self.tmp
        test.nobody         = self.nobody
        test.php_conf       = self.php_conf
        test.proxy_suitable = self.proxy_suitable
        test.is_ssl         = self.is_ssl

        self.tests.append (test)
        return test

    def Empty (self):
        self.tests = []
        self.num   = 0

    def Clean (self):
        for t in self.tests:
            self.current_test = t
            t.Clean()

    def Precondition (self):
        for t in self.tests:
            self.current_test = t
            if t.Precondition() == False:
                return False
        return True

    def Prepare (self, www):
        for t in self.tests:
            self.current_test = t
            t.Prepare(www)

    def JustBefore (self, www):
        for t in self.tests:
            self.current_test = t
            t.JustBefore(www)

    def JustAfter (self, www):
        current = self.current_test

        for t in self.tests:
            self.current_test = t
            t.JustAfter(www)

        self.current_test = current

    def Run (self, host, port):
        for t in self.tests:
            self.current_test = t
            r = t.Run(host, port)
            if r == -1: return r

            if self.delay > 0:
                time.sleep (self.delay)
        return r

    def __str__ (self):
        return str(self.current_test)
