# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
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

AVAILABLE_LANGUAGES = [
    ('en',           N_('English')),
    ('es',           N_('Spanish')),
    ('de',           N_('German')),
    ('fr',           N_('French')),
    ('it',           N_('Italian')),
    ('ja',           N_('Japanese')),
    ('nl',           N_('Dutch')),
    ('pl',           N_('Polish')),
    ('sv_SE',        N_('Swedish')),
    ('po_BR',        N_('Brazilian Portuguese')),
    ('zh_CN',        N_('Chinese Simplified')),
    ('ca',           N_('Catalan')),
    ('gl',           N_('Galician'))
]

PRODUCT_TOKENS = [
    ('',        N_('Default')),
    ('product', N_('Product only')),
    ('minor',   N_('Product + Minor version')),
    ('minimal', N_('Product + Minimal version')),
    ('os',      N_('Product + Platform')),
    ('full',    N_('Full Server string'))
]

HANDLERS = [
    ('',             N_('None')),
    ('common',       N_('List & Send')),
    ('file',         N_('Static Content')),
    ('dirlist',      N_('Only Listing')),
    ('redir',        N_('Redirection')),
    ('fcgi',         N_('FastCGI')),
    ('scgi',         N_('SCGI')),
    ('uwsgi',        N_('uWSGI')),
    ('proxy',        N_('HTTP Reverse Proxy')),
    ('post_report',  N_('Upload Reporting')),
    ('streaming',    N_('Audio/Video Streaming')),
    ('cgi',          N_('CGI')),
    ('ssi',          N_('Server Side Includes')),
    ('secdownload',  N_('Hidden Downloads')),
    ('server_info',  N_('Server Info')),
    ('dbslayer',     N_('MySQL Bridge')),
    ('custom_error', N_('HTTP Error')),
    ('admin',        N_('Remote Administration')),
    ('empty_gif',    N_('1x1 Transparent GIF')),
    ('drop',         N_('Drop Connection')),
]

ERROR_HANDLERS = [
    ('',            N_('Default errors')),
    ('error_redir', N_('Custom redirections')),
    ('error_nn',    N_('Closest match'))
]

VALIDATORS = [
    ('',         N_('None')),
    ('plain',    N_('Plain text file')),
    ('htpasswd', N_('Htpasswd file')),
    ('htdigest', N_('Htdigest file')),
    ('ldap',     N_('LDAP server')),
    ('mysql',    N_('MySQL server')),
    ('pam',      N_('PAM')),
    ('authlist', N_('Fixed list'))
]

VALIDATOR_METHODS = [
    ('basic',        N_('Basic')),
    ('digest',       N_('Digest')),
    ('basic,digest', N_('Basic or Digest'))
]

LOGGERS = [
    ('',           N_('None')),
    ('combined',   N_('Apache compatible')),
    ('ncsa',       N_('NCSA')),
    ('custom',     N_('Custom'))
]

LOGGER_WRITERS = [
    ('file',     N_('File')),
    ('syslog',   N_('System logger')),
    ('stderr',   N_('Standard Error')),
    ('exec',     N_('Execute program'))
]

BALANCERS = [
    ('',            N_('None')),
    ('round_robin', N_("Round Robin")),
    ('ip_hash',     N_("IP Hash")),
    ('failover',    N_("Failover"))
]

SOURCE_TYPES = [
    ('interpreter', N_('Local interpreter')),
    ('host',        N_('Remote host'))
]

ENCODERS = [
    ('gzip',     N_('GZip')),
    ('deflate',  N_('Deflate'))
]

THREAD_POLICY = [
    ('',      N_('Default')),
    ('fifo',  N_('FIFO')),
    ('rr',    N_('Round-robin')),
    ('other', N_('Dynamic'))
]

POLL_METHODS = [
    ('',       N_('Automatic')),
    ('epoll',  'epoll() - Linux >= 2.6'),
    ('kqueue', 'kqueue() - BSD, OS X'),
    ('ports',  'Solaris ports - >= 10'),
    ('poll',   'poll()'),
    ('select', 'select()')
]

REDIR_SHOW = [
    ('1', N_('External')),
    ('0', N_('Internal'))
]

ERROR_CODES = [
    ('400', '400 Bad Request'),
    ('401', '401 Unauthorized'),
    ('402', '402 Payment Required'),
    ('403', '403 Forbidden'),
    ('404', '404 Not Found'),
    ('405', '405 Method Not Allowed'),
    ('406', '406 Not Acceptable'),
    ('407', '407 Proxy Auth Required'),
    ('408', '408 Request Timeout'),
    ('409', '409 Conflict'),
    ('410', '410 Gone'),
    ('411', '411 Length Required'),
    ('412', '412 Precondition Failed'),
    ('413', '413 Request Entity too large'),
    ('414', '414 Request-URI too long'),
    ('415', '415 Unsupported Media Type'),
    ('416', '416 Requested range not satisfiable'),
    ('417', '417 Expectation Failed'),
    ('422', '422 Unprocessable Entity'),
    ('423', '423 Locked'),
    ('424', '424 Failed Dependency'),
    ('425', '425 Unordered Collection'),
    ('426', '426 Upgrade Required'),
    ('449', '449 Retry With'),
    ('500', '500 Internal Server Error'),
    ('501', '501 Not Implemented'),
    ('502', '502 Bad gateway'),
    ('503', '503 Service Unavailable'),
    ('504', '504 Gateway Timeout'),
    ('505', '505 HTTP Version Not Supported'),
    ('506', '506 Variant Also Negotiates'),
    ('507', '507 Insufficient Storage'),
    ('509', '509 Bandwidth Limit Exceeded'),
    ('510', '510 Not Extended')
]

RULES = [
    ('directory',  N_('Directory')),
    ('extensions', N_('Extensions')),
    ('request',    N_('Regular Expression')),
    ('header',     N_('Header')),
    ('exists',     N_('File Exists')),
    ('method',     N_('HTTP Method')),
    ('bind',       N_('Incoming IP/Port')),
    ('tls',        N_('SSL / TLS')),
    ('fullpath',   N_('Full Path')),
    ('from',       N_('Connected from')),
    ('url_arg',    N_('URL Argument')),
    ('geoip',      N_('GeoIP'))
]

VRULES = [
    ('',           N_('Match Nickname')),
    ('wildcard',   N_('Wildcards')),
    ('rehost',     N_('Regular Expressions')),
    ('target_ip',  N_('Server IP'))
]

EXPIRATION_TYPE = [
    ('',         N_('Not set')),
    ('epoch',    N_('Already expired on 1970')),
    ('max',      N_('Do not expire until 2038')),
    ('time',     N_('Custom value'))
]

CRYPTORS = [
    ('',         N_('No TLS/SSL')),
    ('libssl',   N_('OpenSSL / libssl'))
]

EVHOSTS = [
    ('',         N_('Off')),
    ('evhost',   N_('Enhanced Virtual Hosting'))
]

CLIENT_CERTS = [
    ('',         N_('Skip')),
    ('tolerate', N_('Tolerate')),
    ('accept',   N_('Accept')),
    ('require',  N_('Require'))
]

COLLECTORS = [
    ('',         N_('Disabled')),
    ('rrd',      N_('RRDtool graphs'))
]

UTC_TIME = [
    ('',         N_('Local time')),
    ('1',        N_('UTC: Coordinated Universal Time'))
]

DWRITER_LANGS = [
    ('json',     N_('JSON')),
    ('python',   N_('Python')),
    ('php',      N_('PHP')),
    ('ruby',     N_('Ruby'))
]

POST_TRACKERS = [
    ('',           N_('Disabled')),
    ('post_track', N_('POST tracker'))
]

CACHING_OPTIONS = [
    ('',         N_('Not set')),
    ('public',   N_('Public')),
    ('private',  N_('Private')),
    ('no-cache', N_('No Cache'))
]

COMPRESSION_LEVELS = [
    ('',  N_('Default')),
    ('0', N_('0 - No compression')),
    ('1', N_('1')),
    ('2', N_('2')),
    ('3', N_('3')),
    ('4', N_('4')),
    ('5', N_('5')),
    ('6', N_('6')),
    ('7', N_('7')),
    ('8', N_('8')),
    ('9', N_('9 - Max compression'))
]

HANDLERS_WITH_BALANCER = [
    'fcgi', 'scgi', 'uwsgi', 'proxy', 'dbslayer'
]
