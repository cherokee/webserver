
# Ports
PORT              = 1978
PORT_TLS          = 1979
HOST              = "localhost"

# Basic paths
CHEROKEE_PATH     = "../cherokee/cherokee-worker"
CHEROKEE_MODS     = "../cherokee/.libs/"
CHEROKEE_DEPS     = "../cherokee/"
CHEROKEE_PANIC    = "../cherokee/cherokee-panic"
CHEROKEE_THEMES   = "../themes"

# Logging
LOGGER_TYPE       = "combined"
LOGGER_ACCESS     = "access.log"
LOGGER_ERROR      = "error.log"

# TLS/SSL
SSL_CERT_FILE     = "ssl-keys/set-1.pem"
SSL_CERT_KEY_FILE = "ssl-keys/set-1.key"

# Misc options
SERVER_DELAY      = 10

# Third party utilities
PYTHON_PATH       = "/usr/bin/python3"
PHPCGI_PATH       = ""
PHP_FCGI_PORT     = 1980

PHP_DIRS          = ["/usr/lib/cgi-bin/",
                     "/usr/bin/",
                     "/usr/sbin/",
                     "/usr/local/bin/",
                     "/opt/csw/php5/bin/",
                     "/opt/local/bin"]

PYTHON_DIRS       = ["/usr/bin",
                     "/usr/local/bin",
                     "/opt/csw/bin",
                     "/opt/local/bin"]

PHP_NAMES         = ["php5-cgi",
                     "php-cgi",
                     "php4-cgi",
                     "php5",
                     "php",
                     "php4"]

PYTHON_NAMES      = ["python3",
                     "python3.14",
                     "python3.13",
                     "python3.12"]
