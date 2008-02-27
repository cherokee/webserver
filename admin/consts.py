ENTRY_TYPES = [
    ('directory',  'Directory'),
    ('extensions', 'Extensions'),
    ('request',    'Regular Expression')
]

HANDLERS = [
    ('common',      'List & Send'),
    ('file',        'Static content'),
    ('dirlist',     'Only listing'),
    ('redir',       'Redirection'),
    ('cgi',         'CGI'),
    ('fcgi',        'FastCGI'),
    ('scgi',        'SCGI'),
    ('server_info', 'Server Info'),
    ('mirror',      'Generic balancer'),
    ('admin',       'Remote Administration')
]

VALIDATORS = [
    ('',         'None'),
    ('plain',    'Plain text file'),
    ('htpasswd', 'Htpasswd file'),
    ('htdigest', 'Htdigest file'),
    ('ldap',     'LDAP server'),
    ('mysql',    'MySQL server')
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
    ('syslog',   'System logger'),
    ('stderr',   'Standard Error'),
    ('file',     'File'),
    ('exec',     'Execute program')
]

BALANCERS = [
    ('round_robin', "Round Robin")
]

BALANCER_TYPES = [
    ('interpreter', 'Local interpreter'),
    ('host',        'Remote host')
]

ENCODERS = [
    ('gzip',     'GZip')
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


