HELP_PRINT_PARAMETERS = """
Cherokee QA test suit
Usage: %s [parameters] [tests]

  -h            Print this help
  -c            Do NOT clean temporal files
  -k            Do NOT kill the server after the tests
  -f            Do NOT use FastCGI by default
  -q            Quiet mode
  -s            Use SSL
  -x            Run under Strace
  -b            Run server as nobody
  -l            Run server with a log file

  -n<NUM>       Repetitions
  -t<NUM>       Number of threads of the server
  -p<NUM>       Server port
  -r<NUM>       Starting delay (secs)
  -j<FLOAT>     Delay between tests (secs)

  -d<NUM>       Number of pauses
  -e<PATH>      Path to the binary of the server
  -m<STRING>    HTTP method name for the requests
  -v<STRING>    Run under Valdrind, string=['', 'hel', 'cac', 'cal']

Report bugs to alvaro@gnu.org
"""

def help_print_parameters():
    from sys import argv
    print HELP_PRINT_PARAMETERS % (argv[0])
