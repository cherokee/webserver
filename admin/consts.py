# For gettext
N_ = lambda x: x

AVAILABLE_LANGUAGES = [
    ('en',           N_('English')),
    ('es',           N_('Spanish')),
    ('de',           N_('German')),
    ('nl',           N_('Dutch')),
    ('sv_SE',        N_('Swedish')),
    ('zh_CN',        N_('Chinese Simplified'))
]

HANDLERS = [
    ('',             N_('None')),
    ('common',       N_('List & Send')),
    ('file',         N_('Static content')),
    ('dirlist',      N_('Only listing')),
    ('redir',        N_('Redirection')),
    ('fcgi',         N_('FastCGI')),
    ('scgi',         N_('SCGI')),
    ('proxy',        N_('HTTP reverse proxy')),
    ('streaming',    N_('Audio/Video streaming')),
    ('cgi',          N_('CGI')),
    ('ssi',          N_('Server Side Includes')),
    ('secdownload',  N_('Hidden Downloads')),
    ('server_info',  N_('Server Info')),
    ('dbslayer',     N_('MySQL bridge')),
    ('mirror',       N_('Generic balancer')),
    ('custom_error', N_('HTTP error')),
    ('admin',        N_('Remote Administration')),
    ('empty_gif',    N_('1x1 Transparent GIF'))
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
    ('round_robin', N_("Round Robin")),
    ('ip_hash',     N_("IP Hash"))
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
    ('select', 'select()'),
    ('win32',  'Win32')
]

REDIR_SHOW = [
    ('1', N_('External')),
    ('0', N_('Internal'))
]

ERROR_CODES = [
    ('400', '400 Bad Request'),
    ('403', '403 Forbidden'),
    ('404', '404 Not Found'),
    ('405', '405 Method Not Allowed'),
    ('410', '410 Gone'),
    ('413', '413 Request Entity too large'),
    ('414', '414 Request-URI too long'),
    ('416', '416 Requested range not satisfiable'),
    ('500', '500 Internal Server Error'),
    ('501', '501 Not Implemented'),
    ('502', '502 Bad gateway'),
    ('503', '503 Service Unavailable'),
    ('504', '504 Gateway Timeout'),
    ('505', '505 HTTP Version Not Supported')
]

RULES = [
    ('directory',  N_('Directory')),
    ('extensions', N_('Extensions')),
    ('request',    N_('Regular Expression')),
    ('header',     N_('Header')),
    ('exists',     N_('File Exists')),
    ('method',     N_('HTTP Method')),
    ('bind',       N_('Incoming Port')),
    ('fullpath',   N_('Full Path')),
    ('from',       N_('Connected from')),
    ('geoip',      N_('GeoIP'))
]

VRULES = [
    ('',           N_('Choose..')),
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
    ('accept',   N_('Accept')),
    ('required', N_('Require'))
]

COLLECTORS = [
    ('',         N_('Disabled')),
    ('rrd',      N_('RRDtool graphs'))
]
