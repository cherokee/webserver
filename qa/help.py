HELP_PRINT_PARAMETERS = """
Cherokee QA test suit
Usage: %s [parameters] [tests]

  -h            Print this help
  -c            Do NOT clean temporal files
  -k            Do NOT kill the server after the tests
  -q            Quiet mode
  -s            Use SSL
  -x            Run under Strace/Dtruss
  -b            Run server as nobody
  -l            Run server with a log file
  -a            Randomize tests

  -n<NUM>       Repetitions
  -p<NUM>       Server port
  -r<NUM>       Starting delay (secs)
  -t<NUM>       Client threads number
  -T<NUM>       Server threads number
  -j<FLOAT>     Delay between tests (secs)

  -d<NUM>       Number of pauses
  -D<NUM>       Pause before executing test NUM
  -e<PATH>      Path to the binary of the server
  -m<STRING>    HTTP method name for the requests
  -v<STRING>    Run under Valgrind, string=['', 'hel', 'cac', 'cal']
  -P<STRING>    Proxy address: host:ip[:public_ip]

Report bugs to http://bugs.cherokee-project.com/
"""

def help_print_parameters():
    from sys import argv
    print HELP_PRINT_PARAMETERS % (argv[0])
