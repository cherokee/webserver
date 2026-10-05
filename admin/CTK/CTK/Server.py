# -*- coding: utf-8 -*-

# CTK: Cherokee Toolkit
#

from __future__ import annotations
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2009-2014 Alvaro Lopez Ortega
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#

import re
import sys
import types
import errno
import threading
import traceback
import urllib.parse
from contextvars import ContextVar
from functools import cmp_to_key
from typing import Any, Callable, Optional


def cmp(left, right):
    return (left > right) - (left < right)

from . import pyscgi
import http.cookies
from .Config import Config

from .util import json_dump
from .Post import Post
from .HTTP import HTTP_Response, HTTP_Error

from html import escape as escape_html


class RequestContext:
    """Typed request-local state exposed through CTK's global helpers."""

    def __init__(self) -> None:
        self.scgi_conn: Optional[ServerHandler] = None
        self.request_url = ''
        self.post: Optional[Post] = None


_request_context: ContextVar[Optional[RequestContext]] = ContextVar(
    'ctk_request_context', default=None)


def _get_request_context() -> RequestContext:
    context = _request_context.get()
    if context is None:
        context = RequestContext()
        _request_context.set(context)
    return context


class PostValidator:
    def __init__ (self, post, validation_list):
        self.post            = post
        self.validation_list = validation_list

    def Validate (self):
        errors  = {}
        updates = {}
        ret     = "warning"

        for key in self.post:
            for regex, func in self.validation_list:
                if re.match(regex, key):
                    for n in range(len(self.post[key])):
                        val = self.post[key][n]
                        if not val and not getattr(func, 'CHECK_ON_NO_VALUE', False):
                            continue

                        try:
                            tmp = func (val)
                        except UserWarning as e:
                            errors[key] = str(e.args[0]) if e.args else str(e)
                            tmp = str(e.args[1]) if len(e.args) > 1 else val
                        except Exception as e:
                            errors[key] = str(e)
                            ret = "unsatisfactory"
                            break

                        if tmp and tmp != val:
                            self.post[key][n] = tmp
                            updates[key] = tmp

        if errors or updates:
            return {'ret': ret,
                    'errors':  errors,
                    'updates': updates}


class ServerHandler (pyscgi.SCGIHandler):
    def __init__ (self, *args):
        self.response = HTTP_Response()

        # SCGIHandler.__init__ invokes ::handle()
        pyscgi.SCGIHandler.__init__ (self, *args)

    def _process_post (self):
        pyscgi.SCGIHandler.handle_post(self)
        post = Post (self.post or '')
        return post

    def _do_handle (self):
        # Read the URL
        url = self.env['REQUEST_URI']

        # Get a copy of the server (it did fork!)
        server = get_server()

        # Security Checks
        sec_error = False
        if server.use_sec_cookie:
            if not self.env['CTK_COOKIE']:
                sec_error = "Cookie"
            elif not server.sec_cookie:
                server.sec_cookie = self.env['CTK_COOKIE'][:]
            else:
                if server.sec_cookie != self.env['CTK_COOKIE']:
                    sec_error = "Cookie"

        if server.use_sec_submit:
            if not server.sec_submit:
                server.sec_submit = self.env['CTK_SUBMITTER_SECRET'][:]

            if self.env['REQUEST_METHOD'] == 'POST':
                if not server.sec_submit in url:
                    sec_error = "Submit"

        if sec_error:
            response = HTTP_Response (error=403, body=f"{sec_error} check failed")
            self.send (str(response))
            return

        # Keep request state local to this execution context without adding
        # untyped attributes to threading.Thread.
        context = _get_request_context()
        context.scgi_conn = self
        context.request_url = url

        base_path = urllib.parse.urlsplit(url).path
        if len(base_path) > 1 and base_path[-1] == '/':
            base_path = base_path[:-1]  # remove trailing '/' if it exists

        for published in server._web_paths:
            if re.match (published._regex, base_path):
                warnings = {}

                # POST
                if published._method == 'POST':
                    post = self._process_post()
                    context.post = post

                    # Validate
                    validator = PostValidator (post, published._validation)
                    errors = validator.Validate()
                    if errors:
                        if errors['ret'] == 'warning':
                            warnings = errors
                        else:
                            resp = HTTP_Response(200, body=json_dump(errors))
                            resp['Content-Type'] = "application/json"
                            return resp

                # Execute handler
                ret = published (**published._kwargs)

                # Deal with the returned info
                if type(ret) == str:
                    self.response += ret
                    return self.response

                elif type(ret) == dict:
                    info = json_dump(dict(list(ret.items()) + list(warnings.items())))
                    self.response += info
                    self.response['Content-Type'] = "application/json"
                    return self.response

                elif isinstance(ret, HTTP_Response):
                    return ret

                else:
                    self.response += ret
                    return self.response

        # Not found
        return HTTP_Error (404)

    def handle_request (self):
        def manage_exception():
            # Print the backtrace
            info = traceback.format_exc()
            print(info, file=sys.stderr)

            # Custom error management
            if error.page:
                try:
                    page = error.page (info, desc)
                    response = HTTP_Response (error=500, body=page.Render())
                    self.send (str(response))
                    return
                except Exception as e:
                    print("!!!!!!", e)
                    pass

            # No error handling page
            html = f'<pre>{escape_html(info)}</pre>'
            self.send (str(HTTP_Error(desc=html)))

        try:
            content = self._do_handle()
            self.send (str(content))

        except OSError as e:
            if e.errno == errno.EPIPE:
                # The web server closed the SCGI socket
                return
            manage_exception()

        except Exception as desc:
            manage_exception()



class Server:
    def __init__ (self):
        self._web_paths     = []
        self._scgi          = None
        self._is_init       = False
        self.lock           = threading.RLock()
        self.plugin_paths   = []
        self.exiting        = False
        self.use_sec_cookie = False
        self.sec_cookie     = None
        self.use_sec_submit = False
        self.sec_submit     = None

    def init_server (self, *args, **kwargs):
        # Is it already init?
        if self._is_init:
            return
        self._is_init = True

        # Security cookie/submit
        if 'sec_cookie' in kwargs:
            self.use_sec_cookie = kwargs.pop('sec_cookie')

        if 'sec_submit' in kwargs:
            self.use_sec_submit = kwargs.pop('sec_submit')

        # Instance SCGI server
        self._scgi = pyscgi.ServerFactory (*args, **kwargs)

        # Figure plug-in paths
        from .Plugin import figure_plugin_paths
        self.plugin_paths = figure_plugin_paths()

    def sort_routes (self):
        def __cmp(x,y):
            lx = len(x._regex)
            ly = len(y._regex)
            return cmp(ly,lx)

        self.lock.acquire()
        try:
            self._web_paths.sort(key=cmp_to_key(__cmp))
        finally:
            self.lock.release()

    def add_route (self, route_obj):
        self.lock.acquire()
        try:
            # Look for a duplicate
            to_remove = []
            for r in self._web_paths:
                if r._regex == route_obj._regex:
                    to_remove.append (r)

            self._web_paths = [x for x in self._web_paths if x not in to_remove]

            # Insert
            self._web_paths.append (route_obj)
            self.sort_routes()
        finally:
            self.lock.release()

    def remove_route (self, path):
        self.lock.acquire()
        try:
            to_remove = []
            for r in self._web_paths:
                if r._regex == path:
                    to_remove.append (r)

            self._web_paths = [x for x in self._web_paths if x not in to_remove]
        finally:
            self.lock.release()

    def serve_forever (self):
        if self._scgi is None:
            raise RuntimeError('Server has not been initialized')
        try:
            while not self.exiting:
                # Handle request
                self._scgi.handle_request()
                if self.exiting:
                    break
        except KeyboardInterrupt:
            self.exiting = True

        print("\r", "CTK Back-end Server exiting..")
        self._scgi.server_close()


#
# Helpers
#
def cfg_reply_ajax_ok():
    if cfg.has_changed():
        return {'ret':'ok', 'modified': '#save-button'}

    return {'ret':'ok', 'not-modified': '#save-button'}

def cfg_apply_post():
    """Apply entries of the HTTP POST request to the Configuration
    tree"""
    for k in post:
        cfg[k] = post[k]

    return cfg_reply_ajax_ok()


__global_server = None
def get_server():
    global __global_server
    if not __global_server:
        __global_server = Server ()

    return __global_server

def get_scgi():
    """Retrieve SCGI object for current connection"""
    scgi = _get_request_context().scgi_conn
    if scgi is None:
        raise RuntimeError('No active SCGI request')
    return scgi

def init (*args, **kwargs):
    # Init CTK
    from . import Init
    Init.Init()

    # Server
    srv = get_server()

    if not 'threading' in kwargs:
        kwargs['threading'] = True

    kwargs['handler_class'] = ServerHandler
    srv.init_server (*args, **kwargs)

def set_synchronous (sync):
    srv = get_server()
    srv._scgi.set_synchronous (sync)

def add_plugin_dir (path):
    srv = get_server()
    srv.plugin_paths.insert (0, path)

def run (*args, **kwargs):
    init (*args, **kwargs)

    srv = get_server()
    srv.serve_forever()

def stop():
    srv = get_server()
    srv.exiting = True

def step ():
    srv = get_server()
    srv._scgi.handle_request()


class PublishedRoute:
    def __init__(self, func: Callable[..., Any]) -> None:
        self._func = func
        self._kwargs: dict[str, Any] = {}
        self._regex = ''
        self._validation: list[Any] = []
        self._method: Optional[str] = None

    def __call__(self, *args: Any, **kwargs: Any) -> Any:
        return self._func(*args, **kwargs)


def publish (regex_url, klass, **kwargs):
    """Publish a path given by the regex_url regular expression, and
    map it to the class of function given by the klass argument"""
    # Instance object
    if type(klass) == type:
        obj = klass()
    else:
        obj = PublishedRoute (klass)

    # Set internal properties
    obj._kwargs     = kwargs
    obj._regex      = regex_url
    obj._validation = kwargs.pop('validation', [])
    obj._method     = kwargs.pop('method', None)

    # Register
    server = get_server()
    server.add_route (obj)

def unpublish (regex_url):
    """Unpublish a path from the global list of published URLs"""
    # Unregister
    server = get_server()
    server.remove_route (regex_url)


class _Cookie:
    def __setitem__ (self, name, value):
        scgi = get_scgi()
        response = scgi.response
        response['Set-Cookie'] = f"{name}={value}"

    def get_val (self, name, default=None):
        scgi = get_scgi()
        cookie = http.cookies.SimpleCookie(scgi.env.get('HTTP_COOKIE', ''))
        if name in cookie:
            return cookie[name].value
        else:
            return default

    def __getitem__ (self, name):
        return self.get_val (name, None)


class _Post:
    def get_val (self, name, default=None):
        post = _get_request_context().post
        if post is None:
            raise RuntimeError('No active POST request')
        return post.get_val(name, default)

    def __getitem__ (self, name):
        return self.get_val (name, None)

    def __iter__ (self):
        post = _get_request_context().post
        if post is None:
            raise RuntimeError('No active POST request')
        return post.__iter__()

    def __str__ (self):
        post = _get_request_context().post
        if post is None:
            raise RuntimeError('No active POST request')
        return str(post)

    def pop (self, *args):
        post = _get_request_context().post
        if post is None:
            raise RuntimeError('No active POST request')
        return post.pop(*args)

    def keys (self, *args):
        post = _get_request_context().post
        if post is None:
            raise RuntimeError('No active POST request')
        return post.keys(*args)

    def get_all (self, *args):
        post = _get_request_context().post
        if post is None:
            raise RuntimeError('No active POST request')
        return post.get_all(*args)

    def get_raw (self):
        post = _get_request_context().post
        if post is None:
            raise RuntimeError('No active POST request')
        return post.raw_post

class _Request:
    def _get_request_url (self):
        return _get_request_context().request_url

    def _get_request_headers (self):
        return get_scgi().env

    def _get_scgi_conn (self):
        return get_scgi()

    scgi    = property (_get_scgi_conn)
    url     = property (_get_request_url)
    headers = property (_get_request_headers)

class _Error:
    def __init__ (self):
        self.page = None


cookie  = _Cookie()
post    = _Post()
request = _Request()
error   = _Error()
cfg     = Config()
