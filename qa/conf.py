PORT     = 1978
PORT_TLS = 1979
HOST     = "localhost"

CHEROKEE_PATH     = "../cherokee/cherokee-worker"
CHEROKEE_MODS     = "../cherokee/.libs/"
CHEROKEE_DEPS     = "../cherokee/"
CHEROKEE_PANIC    = "../cherokee/cherokee-panic"

LOGGER_TYPE       = "combined"
LOGGER_ACCESS     = "access.log"
LOGGER_ERROR      = "error.log"

STRACE_PATH       = "/usr/bin/strace"
DTRUSS_PATH       = "/usr/bin/dtruss"
VALGRIND_PATH     = "/usr/bin/valgrind"
PYTHON_PATH       = "auto"
PHPCGI_PATH       = "auto"
PHP_FCGI_PORT     = 1980

SSL_CERT_FILE     = "/etc/cherokee/ssl/cherokee.pem"
SSL_CERT_KEY_FILE = "/etc/cherokee/ssl/cherokee.pem"
SSL_CA_FILE       = "/etc/cherokee/ssl/cherokee.pem"

SERVER_DELAY      = 10


# Paths
#
PHP_DIRS          = ["/usr/lib/cgi-bin/",
                     "/usr/bin/",
                     "/usr/local/bin/",
                     "/opt/csw/php5/bin/"]
PYTHON_DIRS       = ["/usr/bin",
                     "/usr/local/bin",
                     "/opt/csw/bin"]
# Names
#
PHP_NAMES         = ["php5-cgi",
                     "php-cgi",
                     "php4-cgi"
                     "php5",
                     "php",
                     "php4"]
PYTHON_NAMES      = ["python2.5",
                     "python2.4",
                     "python",
                     "python2.3"]
