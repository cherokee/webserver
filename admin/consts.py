HANDLERS = [
    ('',             'None'),
    ('common',       'List & Send'),
    ('file',         'Static content'),
    ('dirlist',      'Only listing'),
    ('redir',        'Redirection'),
    ('fcgi',         'FastCGI'),
    ('scgi',         'SCGI'),
    ('proxy',        'HTTP reverse proxy'),
    ('streaming',    'Audio/Video streaming'),
    ('cgi',          'CGI'),
    ('ssi',          'Server Side Includes'),
    ('secdownload',  'Hidden Downloads'),
    ('server_info',  'Server Info'),
    ('dbslayer',     'MySQL bridge'),
    ('mirror',       'Generic balancer'),
    ('custom_error', 'HTTP error'),
    ('admin',        'Remote Administration'),
    ('empty_gif',    '1x1 Transparent GIF')
]

ERROR_HANDLERS = [
    ('',            'Default errors'),
    ('error_redir', 'Custom redirections'),
    ('error_nn',    'Closest match')
]

VALIDATORS = [
    ('',         'None'),
    ('plain',    'Plain text file'),
    ('htpasswd', 'Htpasswd file'),
    ('htdigest', 'Htdigest file'),
    ('ldap',     'LDAP server'),
    ('mysql',    'MySQL server'),
    ('pam',      'PAM'),
    ('authlist', 'Fixed list')
]

VALIDATOR_METHODS = [
    ('basic',        'Basic'),
    ('digest',       'Digest'),
    ('basic,digest', 'Basic or Digest')
]

LOGGERS = [
    ('',           'None'),
    ('combined',   'Apache compatible'),
    ('w3c',        'W3C'),
    ('ncsa',       'NCSA')
]

LOGGER_WRITERS = [
    ('file',     'File'),
    ('syslog',   'System logger'),
    ('stderr',   'Standard Error'),
    ('exec',     'Execute program')
]

BALANCERS = [
    ('round_robin', "Round Robin"),
    ('ip_hash',     "IP Hash")
]

SOURCE_TYPES = [
    ('interpreter', 'Local interpreter'),
    ('host',        'Remote host')
]

ENCODERS = [
    ('gzip',     'GZip'),
    ('deflate',  'Deflate')
]

THREAD_POLICY = [
    ('',      'Default'),
    ('fifo',  'FIFO'),
    ('rr',    'Round-robin'),
    ('other', 'Dynamic')
]

POLL_METHODS = [
    ('',       'Automatic'),
    ('epoll',  'epoll() - Linux >= 2.6'),
    ('kqueue', 'kqueue() - BSD, OS X'),
    ('ports',  'Solaris ports - >= 10'),
    ('poll',   'poll()'),
    ('select', 'select()'),
    ('win32',  'Win32')
]

REDIR_SHOW = [
    ('1', 'External'),
    ('0', 'Internal')
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
    ('directory',  'Directory'),
    ('extensions', 'Extensions'),
    ('request',    'Regular Expression'),
    ('header',     'Header'),
    ('exists',     'File Exists'),
    ('method',     'HTTP Method'),
    ('bind',       'Incoming Port'),
    ('geoip',      'GeoIP')
]

EXPIRATION_TYPE = [
    ('',         'Not set'),
    ('epoch',    'Already expired on 1970'),
    ('max',      'Do not expire until 2038'),
    ('time',     'Custom value')
]

CRYPTORS = [
    ('',         'No TLS/SSL'),
    ('libssl',   'OpenSSL / libssl')
]

CLIENT_CERTS = [
    ('',         'Skip'),
    ('accept',   'Accept'),
    ('required', 'Require')
]
