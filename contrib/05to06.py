#!/usr/bin/env python

##
## Cherokee 0.5.x to 0.6.x configuration converter
##
## Copyright: Alvaro Lopez Ortega <alvaro@alobbs.com>
## Licensed: GPL v2
##

import sys
import string

FAKED_DOCUMENT_ROOT = '/faked'


class Syntax:
    srv_entries =     {'port':                  'int',
                       'porttls':               'int',
                       'ipv6':                  'int',
                       'timeout':               'int',
                       'keepalive':             'int',
                       'maxkeepaliverequests':  'int',
                       'logflushinterval':      'int',
                       'maxfds':                'int',
                       'maxconnectionreuse':    'int',
                       'listenqueuesize':       'int',
                       'servertokens':          'str',
                       'listen':                'str',
                       'user':                  'str',
                       'group':                 'str',
                       'pollmethod':            'str',
                       'pidfile':               'path',
                       'mimefile':              'path',
                       'chroot':                'path',
                       'panicaction':           'path'
                       }

    vsrv_entries =    {'directoryindex':        'list',
                       'sslcertificatefile':    'path',
                       'sslcertificatekeyfile': 'path',
                       'sslcalistfile':         'path'
                       }

    icons_entries =    {'directory':            'str',
                        'parentdirectory':      'str',
                        'default':              'str'}

    similar_entries = {'maxkeepaliverequests':  'keepalive_max_requests',
                       'servertokens':          'server_tokens',
                       'pidfile':               'pid_file',
                       'mimefile':              'mime_file',
                       'documentroot':          'document_root',
                       'errorhandler':          'error_handler',
                       'accesslog':             'access_log',
                       'errorlog':              'error_log',
                       'name':                  'realm',
                       'logflushinterval':      'log_flush_elapse',
                       'maxfds':                'max_fds',
                       'maxconnectionreuse':    'max_connection_reuse',
                       'panicaction':           'panic_action',
                       'pollmethod':            'poll_method',
                       'listenqueuesize':       'listen_queue_size',
                       'sslcertificatefile':    'ssl_certificate_file',
                       'sslcertificatekeyfile': 'ssl_certificate_key_file',
                       'sslcalistfile':         'ssl_cal_list_file',
                       'directoryindex':        'directory_index',
                       'scriptalias':           'script_alias'
                       }
    
    def __init__ (self, lex):
        self._lex = lex

    def _process_entry_guts (self, prefix):
        kind, val = self._lex.get_token()
        if kind == 'handler':
            kind, val = self._lex.get_token()
            if kind == 'file':
                kind = 'str'
                val  = 'file'
            if kind != 'str':
                raise "Expected a str"
            print '%s!handler = %s' % (prefix, val)

            kind, val = self._lex.get_token()
            if kind != '{': 
                self._lex.rewind()
                return True

            regex_num = 1
            while True:
                kind, val_prop = self._lex.get_token()

                # Special cases
                if kind == '}':
                    break
                
                elif kind == 'server':
                    kind, val_prop_val = self._lex.get_token()
                    print '%s!handler!balancer = round_robin' % (prefix)
                    print '%s!handler!balancer!type = interpreter' % (prefix)
                    print '%s!handler!balancer!entry1!host = %s' % (prefix, val_prop_val)

                    kind, val = self._lex.get_token()
                    if kind != '{':
                        self._lex.rewind()
                        continue

                    while True:
                        kind, val = self._lex.get_token()
                        if kind == '}':
                            break
                        elif kind == 'str':
                            kind_prop, val_prop = self._lex.get_token()

                            val = val.lower()
                            if val == 'env':
                                kind_prop_val, val_prop_val = self._lex.get_token()
                                print "%s!handler!balancer!entry1!env!%s = %s" % (prefix, val_prop, val_prop_val)

                            else:
                                print "%s!handler!balancer!entry1!%s = %s" % (prefix, val, val_prop)
                    continue

                elif kind == 'str':
                    val_prop_low = val_prop.lower()
                    if val_prop_low == 'justabout':
                        print '%s!handler!just_about = 1' % (prefix)
                        continue 

                    elif val_prop_low == 'env':
                        kind, val_env_name = self._lex.get_token()
                        if kind != 'str': raise "Expected a str"

                        kind, val_env_val = self._lex.get_token()
                        if kind != 'str': raise "Expected a str"

                        print '%s!handler!env!%s = %s' % (prefix, val_env_name, val_env_val)
                        continue 

                    elif val_prop_low == 'show' or \
                         val_prop_low == 'rewrite':
                        kind, val_prop = self._lex.get_token()

                        # dirlist
                        if val_prop_low == 'show':
                            if 'size'  in val_prop or \
                               'date'  in val_prop or \
                               'owner' in val_prop:
                                print '%s!handler!size = %d' % (prefix, 'size' in val_prop)
                                print '%s!handler!date = %d' % (prefix, 'date' in val_prop)
                                print '%s!handler!user = %d' % (prefix, 'owner' in val_prop)
                                continue
                            
                        # redir
                        rewrite = (kind == 'str' and val_prop.lower() == 'rewrite')
                        if not rewrite: self._lex.rewind()

                        print '%s!handler!rewrite!%d!show = %d' % (prefix, regex_num, rewrite)

                        kind, regex_val = self._lex.get_token()
                        if kind != 'str': raise "Expected a str"

                        kind, url_val = self._lex.get_token()
                        if kind == '}':
                            self._lex.rewind()
                            print '%s!handler!rewrite!%d!substring = %s' % (prefix, regex_num, regex_val)
                            regex_num = regex_num + 1
                            continue

                        elif kind == 'str':
                            print '%s!handler!rewrite!%d!regex = %s' % (prefix, regex_num, regex_val)
                            print '%s!handler!rewrite!%d!substring = %s' % (prefix, regex_num, url_val)
                            regex_num = regex_num + 1
                            continue

                        else:
                            raise "Expected a str or '}'"
                    
                # Default case
                val_prop_low = val_prop.lower()
                if val_prop_low in self.similar_entries:
                    val_prop = self.similar_entries[val_prop_low]
                else:
                    val_prop = val_prop_low                    

                # Get property value
                kind, val_prop_val = self._lex.get_token()
                print '%s!handler!%s = %s' % (prefix, val_prop, val_prop_val)

        elif kind == 'documentroot':
            kind, val = self._lex.get_token()
            if kind != 'path': raise "Malformed DocumentRoot"
            print '%s!document_root = %s' % (prefix, val)
            
        elif kind == 'auth':
            return self._process_auth (prefix)

        elif kind == 'allow':
            kind, val = self._lex.get_token()
            if kind != 'str': raise "Malformed Allow"

            kind, val_ip = self._lex.get_token()
            if not kind in ['str', 'list']: raise "Malformed Allow from"

            print '%s!allow_from = %s' % (prefix, val_ip)
            
        else:
            self._lex.rewind()
            return False

        return True

    def _process_auth (self, prefix):
        kind, val_method = self._lex.get_token()
        if kind not in ['str', 'list']: raise "Malformed Auth"

        val_method = val_method.lower()
        print "%s!auth!methods = %s" % (prefix, val_method)

        kind, val = self._lex.get_token()
        if kind != '{': raise "Malformed Auth"

        while True:
            kind, val_prop_name = self._lex.get_token()

            if kind == 'user':
                kind = 'str'
                val_prop_name = 'user'
            
            if kind == '}':
                self._lex.rewind()
                break
            elif kind != 'str':
                raise "Malformed Auth"

            kind, val_prop = self._lex.get_token()
            if kind != 'str': raise "Malformed Auth"

            val_prop_name_low = val_prop_name.lower()
            if val_prop_name_low in self.similar_entries:
                val_prop_name = self.similar_entries[val_prop_name_low]
            else:
                val_prop_name = val_prop_name_low

            if val_prop_name == 'method':
                print "%s!auth = %s" % (prefix, val_prop)
            else:
                print "%s!auth!%s = %s" % (prefix, val_prop_name, val_prop)

            kind, val = self._lex.get_token()
            if kind == '{':
                while True:
                    kind, val_prop = self._lex.get_token()
                    if kind == '}':
                        break
                    else:
                        val_prop = val_prop.lower()

                    kind, val_val = self._lex.get_token()
                    print "%s!auth!%s = %s" % (prefix, val_prop, val_val)
            else:
                self._lex.rewind()            

        kind, val = self._lex.get_token()
        if kind != '}' : raise "Malformed Auth"
        return True

    def _process_entry (self, prefix):
        while True:
            more = self._process_entry_guts (prefix)
            if not more: break

        try:
            priority = self._vserver_last_priority * 10
        except AttributeError:
            self._vserver_last_priority = 1
            priority = 10
        print "%s!priority = %d" % (prefix, priority * 10)
        self._vserver_last_priority += 1

    def _process_encoder (self, prefix):
        while True:
            kind, val = self._lex.get_token()
            if kind not in ['allow', 'deny']: 
                raise "Malformed encoder"

            kind_val, val_val = self._lex.get_token()
            if kind_val != 'list': raise "Malformed encoder"

            print "%s!%s = %s" % (prefix, kind, val_val)

            kind, val = self._lex.get_token()
            self._lex.rewind()
            if kind == '}':
                break

    def _process_log (self, prefix):
        while True:
            kind, val1 = self._lex.get_token()
            if kind == '}':
                self._lex.rewind()
                break
            if kind != 'str': raise "Malformed Log"

            val1 = val1.lower()

            if val1.startswith('access'):
                entry = 'access'
            elif val1.startswith('error'):
                entry = 'error'
            else:
                raise "Unknown Log entry " + val1

            kind, val2 = self._lex.get_token()
            if kind != 'path': raise "Malformed Log"
            
            if val2[0] == '/' or val2[1:2] == ':\\':
                print "%s!%s!type = file" % (prefix, entry)
                print "%s!%s!filename = %s" % (prefix, entry, val2)
            elif val == 'syslog':
                print "%s!%s!type = syslog" % (prefix, entry)

    def _process_icons_content (self):
        kind, val = self._lex.get_token()
        if kind is None:
            return False

        if kind in self.icons_entries:
            val_kind, val_val = self._lex.get_token()
            expected_type = self.icons_entries[kind]
            if val_kind != expected_type:
                raise "Bad %s value: type=%s expected=%s" % \
                      (kind, val_kind, expected_type)

            if kind in self.similar_entries:
                kind = self.similar_entries[kind]

            print "icons!%s = %s" % (kind, str(val_val))

        elif kind == 'file':
            kind, val = self._lex.get_token()
            if kind != '{': raise "Malformed Icon file entry"

            while True:
                kind, val_file = self._lex.get_token()
                if kind == '}':
                    break
                elif kind != 'str':
                    raise "Malformed Icon file entry"

                kind, val_pattern = self._lex.get_token()
                if kind != 'str':
                    raise "Malformed Icon file entry"

                print "icons!file!%s = %s" % (val_file, val_pattern)

        elif kind == 'suffix':
            kind, val = self._lex.get_token()
            if kind != '{': raise "Malformed Icon suffix entry"

            while True:
                kind, val_file = self._lex.get_token()
                if kind == '}':
                    break
                elif kind != 'str':
                    raise "Malformed Icon suffix entry"

                kind, val_suffixes = self._lex.get_token()
                if kind not in ['str', 'list']:
                    raise "Malformed Icon suffix entry"

                print "icons!suffix!%s = %s" % (val_file, val_suffixes)

        else:
            self._lex.rewind()            
            return False

        return True

    def _process_virtual_server_content (self, vserver='default'):
        kind, val = self._lex.get_token()
        if kind is None:
            return False

        if kind in self.vsrv_entries:
            val_kind, val_val = self._lex.get_token()
            expected_type = self.vsrv_entries[kind]
            if val_kind != expected_type:
                raise "Bad %s value: type=%s expected=%s" % \
                      (kind, val_kind, expected_type)

            if kind in self.similar_entries:
                kind = self.similar_entries[kind]

            print "vserver!%s!%s = %s" % (vserver, kind, str(val_val))

        elif kind == 'documentroot':
            kind, val = self._lex.get_token()
            if kind != 'path': raise "Malformed DocumentRoot"

            print "vserver!%s!document_root = %s" % (vserver, val)
            self._vserver_has_document_root = True

        elif kind == 'directory':
            kind, dir_val = self._lex.get_token()

            if kind == 'str':
                # If the token kind is a string, it means that
                # it's processing an icon. Return and let the
                # icon management method to take care for us.
                return False

            elif kind != 'path':
                raise "Malformed directory"

            kind, val = self._lex.get_token()
            if kind != '{': raise "Malformed directory"

            prefix = 'vserver!%s!directory!%s' % (vserver, dir_val)
            self._process_entry (prefix)

            kind, val = self._lex.get_token()
            if kind != '}': raise "Malformed directory", self._lex._txt

        elif kind == 'extension':
            kind, ext_val = self._lex.get_token()
            if not kind in ['list', 'str']: raise "Malformed extension"

            kind, val = self._lex.get_token()
            if kind != '{': raise "Malformed extension"

            prefix = 'vserver!%s!extensions!%s' % (vserver, ext_val)
            self._process_entry (prefix)

            kind, val = self._lex.get_token()
            if kind != '}': raise "Malformed extension"

        elif kind == 'request':
            kind, req_val = self._lex.get_token()
            if kind != 'str': raise "Malformed request"

            kind, val = self._lex.get_token()
            if kind != '{': raise "Malformed request"

            prefix = 'vserver!%s!request!%s' % (vserver, req_val)
            self._process_entry (prefix)

            kind, val = self._lex.get_token()
            if kind != '}': raise "Malformed request"

        elif kind == 'userdir':
            kind, pub_val = self._lex.get_token()
            if kind != 'str': raise "Malformed Userdir"

            kind, val = self._lex.get_token()
            if kind != '{': raise "Malformed Userdir"

            prefix = 'vserver!%s!user_dir!%s' % (vserver, pub_val)
            while True:
                more = self._process_virtual_server_content (prefix)
                if not more: break

            kind, val = self._lex.get_token()
            if kind != '}': raise "Malformed Userdir"

        elif kind == 'errorhandler':
            kind, handler_val = self._lex.get_token()
            if kind != 'str': raise "Malformed ErrorHandler"
            
            kind, val = self._lex.get_token()
            if kind != '{': raise "Malformed ErrorHandler"

            prefix = 'vserver!%s!error_handler' % (vserver)
            print "%s = %s" % (prefix, handler_val)

            while True:
                kind, error_val = self._lex.get_token()
                if kind == '}':
                    self._lex.rewind()
                    break
                elif kind != 'int':
                    raise "Malformed ErrorHandler"

                kind, url_val = self._lex.get_token()
                if kind != 'str':
                    raise "Malformed ErrorHandler"

                print "%s!%s = %s" % (prefix, error_val, url_val)
            
            kind, val = self._lex.get_token()
            if kind != '}': raise "Malformed ErrorHandler"

        elif kind == 'log':
            kind, log_val = self._lex.get_token()
            if kind != 'str': raise "Malformed Log"
            
            kind, val = self._lex.get_token()
            if kind != '{':
                self._lex.rewind()
                return True

            prefix = 'vserver!%s!logger' % (vserver)
            print "%s = %s" % (prefix, log_val)
            self._process_log (prefix)

            kind, val = self._lex.get_token()
            if kind != '}': raise "Malformed Log"

        else:
            self._lex.rewind()            
            return False

        return True

    def _process_server_content (self):
        kind, val = self._lex.get_token()
        if kind is None: return False

        if kind in self.srv_entries:
            val_kind, val_val = self._lex.get_token()
            expected_type = self.srv_entries[kind]
            if val_kind != expected_type:
                raise "Bad %s value: type=%s expected=%s" % \
                      (kind, val_kind, expected_type)

            if kind in self.similar_entries:
                kind = self.similar_entries[kind]

            print "server!%s = %s" % (kind, str(val_val))

        elif kind == 'icons':
            kind, icons_val = self._lex.get_token()
            if kind != 'path': raise "Malformed Icons"

        elif kind == 'include':
            kind, incl_val = self._lex.get_token()
            if kind != 'path': raise "Malformed Include"

            print "include = %s" % (incl_val)

        elif kind == 'encoder':
            kind, enc_val = self._lex.get_token()
            if kind != 'str': raise "Malformed encoder"
            
            kind, val = self._lex.get_token()
            if kind != '{': raise "Malformed encoder"

            prefix = 'server!encoder!%s' % (enc_val)
            self._process_encoder (prefix)

            kind, val = self._lex.get_token()
            if kind != '}': raise "Malformed encoder"

        elif kind == 'server':
            self._lex.rewind()
            self._process_server()

        elif kind == 'threadnumber':
            kind, val_num = self._lex.get_token()
            if kind != 'int': raise "Malformed threadnumber"

            print "server!thread_number = %s" % (val_num)

            kind, val = self._lex.get_token()
            if kind != '{':
                self._lex.rewind()
                return True

            kind, val_type = self._lex.get_token()
            if kind != 'policy':
                raise "Malformed threadnumber"
            
            kind, val_val = self._lex.get_token()
            if kind != 'str': raise "Malformed threadnumber"

            print "server!thread_policy = %s" % (val_val)

            kind, val = self._lex.get_token()
            if kind != '}': raise "Malformed threadnumber"

        elif kind == 'sendfile':
            kind, val = self._lex.get_token()
            if kind != '{': raise "Malformed Sendfile"

            while True:
                kind, val = self._lex.get_token()
                if kind == '}':
                    break
                elif kind != 'str':
                    raise "Malformed Sendfile"

                val = val.lower()
                
                if val == 'minsize':
                    kind, val_num = self._lex.get_token()
                    if kind != 'int': raise "Malformed Sendfile"

                    print "server!sendfile_min = %s" % (val_num)

                elif val == 'maxsize':
                    kind, val_num = self._lex.get_token()
                    if kind != 'int': raise "Malformed Sendfile"

                    print "server!sendfile_max = %s" % (val_num)
                    
                else:
                    raise "Malformed Sendfile"

        else:
            self._lex.rewind()            
            return False

        return True

    def _process_server (self):
        self._vserver_last_priority = 1

        kind, val = self._lex.get_token()
        if kind != 'server': raise "Expected server"

        kind, val = self._lex.get_token()
        if kind not in ['list', 'str']: raise "Malformed server"

        if kind == 'list':
            l = map(lambda x: x.strip(), val.split(','))
            domains = l
        else:
            domains = [val]

        vserver = domains[0]

        kind, val = self._lex.get_token()
        if kind != '{': raise "Expected {"

        i = 0
        for v in domains:
            i = i + 1
            print "vserver!%s!domain!%d = %s" % (vserver, i, v)

        self._vserver_has_document_root = False

        while True:
            more = self._process_virtual_server_content (vserver)
            if not more: break

        kind, val = self._lex.get_token()
        if kind != '}': raise "Expected }, got %s=%s" % (kind,val)

        if not self._vserver_has_document_root:
            print "vserver!%s!document_root = %s" % (vserver, FAKED_DOCUMENT_ROOT)

        print

    def process_sentence (self):
        kind, val = self._lex.get_token()
        if kind == None: return False
        self._lex.rewind()
        
        # Global server stuff
        ok = self._process_server_content ()
        if ok: return True

        # Default virtual server
        ok = self._process_virtual_server_content ()
        if ok: return True

        # Icons
        ok = self._process_icons_content ()
        if ok: return True

        # Unknown: report error
        kind, val = self._lex.get_token()
        raise "Unknown token: %s = %s" % (kind, str(val))


    def convert (self):
        print '# Cherokee 0.6.x configuration file generated by 05to06.py\n'
        print '# Copyright (C) 2006  Alvaro Lopez Ortega <alvaro@alobbs.com>\n'

        print '# This program is distributed in the hope that it will be useful,'
        print '# but WITHOUT ANY WARRANTY, to the extent permitted by law; without'
        print '# even the implied warranty of MERCHANTABILITY or FITNESS FOR A'
        print '# PARTICULAR PURPOSE.\n\n'

        while True:
            more = self.process_sentence()
            if not more:
                break


class Lexer:
    special_strings = ['server_info']

    reserved_words = ['{', '}', 'porttls', 'port', 'directoryindex', 'directory', 'extension',
                      'request', 'handler', 'ipv6', 'timeout', 'keepalive', 'logflushinterval', 
                      'maxkeepaliverequests', 'servertokens', 'encoder', 'log', 'server',
                      'allow', 'deny', 'pidfile', 'icons', 'mimefile', 'errorhandler', 'auth',
                      'include', 'listenqueuesize', 'listen', 'userdir', 'user','group', 'chroot', 
                      'maxfds', 'maxconnectionreuse', 'panicaction', 'pollmethod', 'documentroot',
                      'threadnumber', 'policy', 'sendfile', 'file', 'suffix', 'parentdirectory',
                      'default', 'sslcertificatefile', 'sslcertificatekeyfile', 'sslcalistfile']

    def __init__ (self, txt):
        self._txt      = txt
        self._last     = None,None
        self._rewinded = False
        
    def _find_consume (self, s):
        found = (self._txt.lower().find (s.lower()) == 0)
        if not found: return False

        self._txt = self._txt[len(s):]
        return True

    def _get_word (self):
        if len(self._txt) == 0:
            return ''

        word = ''

        # Quoted string
        if self._txt[0] == '"':
            word += self._txt[0]
            self._txt = self._txt[1:]
            while True:
                word += self._txt[0]
                self._txt = self._txt[1:]
                if word[-2] != '\\':
                    if word[-1] == '"' or len(self._txt) is 0: break
            return word

        # Common word
        while True:
            while self._txt[0] not in " \t\r\n":
                word += self._txt[0]
                self._txt = self._txt[1:]
                if len(self._txt) is 0: break
            if word[-1] != ',': break
            while self._txt[0] in " \t":
                self._txt = self._txt[1:]
        return word
        
    def rewind (self):
        self._rewinded = True

    def get_token (self):
        if self._rewinded:
            self._rewinded = False
            return self._last
        
        self._last = self._get_token_guts()
        return self._last

    def _get_token_guts (self):
        self._txt = self._txt.strip()

        # Special cases
        for word in self.special_strings:
            if self._find_consume(word):
                return 'str',word

        # Reserved words
        for word in self.reserved_words:
            if self._find_consume(word):
                return word,None

        # Int
        int_val = None

        word = self._get_word()
        if len(word) is 0:
            return None, None

        try:
            int_val = int(word)
        except:
            pass

        if int_val is not None:
            return 'int',int_val

        # Path
        if word[0] == '/' or word[1:3] in [':\\', ':/']:
            return 'path',word

        # Quoted string
        if word[0] == word[-1] == '"':
            return 'str',word[1:-1]

        # List
        if word.find(',') != -1:
            return 'list',word

        # Boolean
        if word.lower() == 'on': return 'int',1
        if word.lower() == 'off': return 'int',0

        # Name
        is_name = True
        for c in word:
            if not c in string.letters and \
               not c in string.digits and \
               not c in "._-&?=:\\/*":
                is_name = False
        if is_name:
            return 'str',word

        raise "Couldn't extract token " + word


class Converter:
    def __init__ (self, fin, fout):
        self._in  = fin
        self._out = fout

    def convert (self):
        tmp = filter (lambda x: len(x) > 0 and x[0] != '#',
                      map (lambda x: x.strip(),
                           self._in.readlines()))
        if not len(tmp): return 

        incoming = reduce (lambda x,y: x+' '+y, tmp)
        self.lex = Lexer (incoming)
        self.syn = Syntax (self.lex)

        self.syn.convert()

def main ():
    c = Converter (sys.stdin, sys.stdout)
    c.convert()
    
if __name__ == "__main__":
    main()
