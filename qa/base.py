# Cherokee QA Tests
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2008 Alvaro Lopez Ortega
# This file is distributed under the GPL license.

import os
import imp
import sys
import types
import socket
import string
import tempfile

from conf import *
from util import *

DEFAULT_READ = 8192

def importfile(path):
    filename = os.path.basename(path)
    name, ext = os.path.splitext(filename)

    file = open(path, 'r')
    module = imp.load_module(name, file, path, (ext, 'r', imp.PY_SOURCE))
    file.close()
    
    return module

class TestBase:
    def __init__ (self):
        self.name                    = None    # Test 01: Basic functionality
        self.conf                    = None    # Directory /test { .. }
        self.request                 = ""      # GET / HTTP/1.0
        self.post                    = None
        self.expected_error          = None
        self.expected_content        = None
        self.expected_content_length = None
        self.forbidden_content       = None
        self._initialize()
        
    def _initialize (self):
        self.ssl                     = None
        self.reply                   = ""      # "200 OK"..
        self.version                 = None    # HTTP/x.y: 9, 0 or 1
        self.reply_err               = None    # 200

    def _safe_read (self, s):
         while True: 
            try:
                if self.ssl:
                    return self.ssl.read (DEFAULT_READ)
                else:
                    return s.recv (DEFAULT_READ)
            except socket.error, (err, strerr):
                if err == errno.EAGAIN or \
                   err == errno.EWOULDBLOCK or \
                   err == errno.EINPROGRESS:
                    continue
            raise

    def _do_request (self, port, ssl):
        for res in socket.getaddrinfo(HOST, port, socket.AF_UNSPEC, socket.SOCK_STREAM):
            af, socktype, proto, canonname, sa = res

            try:
                s = socket.socket(af, socktype, proto)
            except socket.error, msg:
                continue

            try:
                s.connect(sa)
            except socket.error, msg:
                s.close()
                s = None
                continue
            break    

        if s is None:
            raise Exception("Couldn't connect to the server")

        if ssl:
            try:
                self.ssl = socket.ssl (s)
            except:
                raise Exception("Couldn't handshake SSL")

        request = self.request + "\r\n"
        if self.post is not None:
            request += self.post

        if self.ssl:
            n = self.ssl.write (request)
        else:
            n = s.send (request)
        assert (n == len(request))
        
        while True:
            try:
                d = self._safe_read (s)
            except Exception, e:
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
            raise Exception("Invalid header, len=%d: '%s'" % (len(reply), reply))

        # Error code
        reply = reply[9:]
        try:
            self.reply_err = int (reply[:3])
        except:
            raise Exception("Invalid header, version=%d len=%d: '%s'" % (self.version, len(reply), reply))

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

        if self.expected_content_length != None:
            if len(self.reply) != self.expected_content_length:
                return -1

        if self.expected_content != None:
            if type(self.expected_content) == types.StringType:
                r = self._check_result_expected_item (self.expected_content)
                if r == -1: return -1
            elif type(self.expected_content) == types.ListType:
                for entry in self.expected_content:
                    r = self._check_result_expected_item (entry)
                    if r == -1: return -1
            else:
                raise Exception("Syntax error")

        if self.forbidden_content != None:
            if type(self.forbidden_content) == types.StringType:
                r = self._check_result_forbidden_item (self.forbidden_content)
                if r == -1: return -1
            elif type(self.forbidden_content) == types.ListType:
                for entry in self.forbidden_content:
                    r = self._check_result_forbidden_item (entry)
                    if r == -1: return -1
            else:
                raise Exception("Syntax error")

        r = self.CustomTest()
        if r == -1: return -1
	                   
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

    def Run (self, port, ssl):
        self._do_request(port, ssl)
        self._parse_output()
        return self._check_result()

    def __str__ (self):
        src = "\tName     = %s\n" % (self.name)

        if self.version == 9:
            src += "\tProtocol = HTTP/0.9\n"
        elif self.version == 0:
            src += "\tProtocol = HTTP/1.0\n"
        elif self.version == 1:
            src += "\tProtocol = HTTP/1.1\n"

        if self.conf is not None:
            src += "\tConfig   = %s\n" % (self.conf)

        header_full = string.split (self.reply,  "\r\n\r\n")[0]
        headers     = string.split (header_full, "\r\n")
        requests    = string.split (self.request, "\r\n")

        src += "\tRequest  = %s\n" % (requests[0])
        for request in requests[1:]:
            if len(request) > 1:
                src += "\t\t%s\n" %(request)

        if self.post is not None and not self.nobody:
            src += "\tPost     = %s\n" % (self.post)

        if self.expected_error is not None:
            src += "\tExpected = Code: %d\n" % (self.expected_error)
        else:
            src += "\tExpected = Code: UNSET!\n"

        if self.expected_content_length is not None:
            src += "\tExpected = Content length: %d\n" % (self.expected_content_length)

        if self.expected_content is not None:
            src += "\tExpected = Content: %s\n" % (self.expected_content)

        if self.forbidden_content is not None:
            src += "\tForbidden= Content: %s\n" % (self.forbidden_content)

        src += "\tReply    = %s\n" % (headers[0])
        for header in headers[1:]:
            src += "\t\t%s\n" %(header)

        if not self.nobody:
            body = self.reply[len(header_full)+4:]
            src += "\tBody len = %d\n" % (len(body))
            src += "\tBody     = %s\n" % (body)

        return src

    def Mkdir (self, www, dir, mode=0777):
        fulldir = os.path.join (www, dir)
        os.makedirs(fulldir, mode)
        return fulldir

    def WriteFile (self, www, filename, mode=0444, content=''):
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
            os.removedirs (fullpath)
            
    def WriteTemp (self, content):
        while True:
            name = self.tmp + "/%s" % (letters_random(40))
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
            from md5 import md5

            md5obj = md5()
            md5obj.update("%s:%s:%s" % (user, realm, passwd))
            a1 = md5obj.hexdigest()

            md5obj = md5()
            md5obj.update("%s:%s" % (method, url))
            ha2 = md5obj.hexdigest()
            
            md5obj = md5()
            md5obj.update("%s:%s:" % (a1, nonce))
            
            if qop != None:
                md5obj.update("%s:" %(nc))
                md5obj.update("%s:" %(cnonce))
                md5obj.update("%s:" %(qop))
            
            md5obj.update(ha2)
            final = md5obj.hexdigest()

            return final


class TestCollection:
    def __init__ (self):
        self.tests = []
        self.num   = 0

    def Add (self, test):
        self.num += 1

        if (test.name == None) or len(test.name) == 0:
            test.name = self.name + ", Part %d" % (self.num)

        test.tmp      = self.tmp
        test.nobody   = self.nobody 
        test.php_conf = self.php_conf

        self.tests.append (test)
        return test

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

    def Run (self, port, ssl):
        for t in self.tests:
            self.current_test = t
            r = t.Run(port, ssl)

            if r == -1: return r
        return r

    def __str__ (self):
        return str(self.current_test)


    

    
    
