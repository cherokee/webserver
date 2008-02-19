ENTRY_TYPES = [
    ('directory',  'Directory'),
    ('extensions', 'Extensions'),
    ('request',    'Regular Expression')
]

HANDLERS = [
    ('common',      'List & Send'),
    ('file',        'Static content'),
    ('dirlist',     'Only listing'),
    ('cgi',         'CGI'),
    ('fcgi',        'FastCGI'),
    ('scgi',        'SCGI'),
    ('server_info', 'Server Info'),
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
