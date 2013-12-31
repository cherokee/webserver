import os, re, sys

# Cherokee Project:
# Error generation and check
#
# This script loads a error definition file, and generates .h and .c
# from it. It also checks whether all the errors are actually being
# used, look for undefined errors, and check that the number of
# parameters matches between the error definition and the source code.
#

SOURCE_DIRS = ['.', '../cget']

#
# Error reading
#
class CherokeeError:
    def __init__ (self, **kwargs):
        self.id    = kwargs['id']
        self.title = kwargs['title']

        self.description = kwargs.get('desc', '').replace('\n',' ')
        self.url_admin   = kwargs.get('admin', '')
        self.help        = kwargs.get('help', [])
        self.debug       = kwargs.get('debug', '')
        self.show_bt     = kwargs.get('show_bt', True)

_errors = []

def e (error_id, title, **kwargs):
    global _errors

    # Check dup. errors
    for err in _errors:
        if error_id == err.id:
            raise ValueError, "ERROR: Duplicated error %s" %(error_id)

    # New error
    kwargs['id']    = error_id
    kwargs['title'] = title
    error = CherokeeError (**kwargs)
    _errors.append (error)


def read_errors():
    exec (open ("error_list.py", 'r').read())


def check_source_code (dirs):
    # Defined errors are unseed
    for def_e in _errors:
        def_e.__seen_in_grep = False

    # Grep the source code
    errors_seen = {}
    for d in dirs:
        for f in os.listdir(d):
            fullpath = os.path.join (d, f)
            if not fullpath.endswith('.c') or \
               not os.path.isfile(fullpath):
                continue

            # Check the source code
            content = open (fullpath, 'r').read()
            for e in re.findall (r'CHEROKEE_ERROR_([\w_]+)[ ,)]', content):
                errors_seen[e] = fullpath

                # Mark used object
                for def_e in _errors:
                    if e == def_e.id:
                        def_e.__seen_in_grep = True

    # Undefined errors in the source code
    error_found = False

    for s in errors_seen.keys():
        found = False
        for e in _errors:
            if s == e.id:
                found = True
                break
        if not found:
            print >> sys.stderr, "Undefined Error: CHEROKEE_ERROR_%s, used in %s" % (s, errors_seen[s])
            error_found = True

    # Unused errors in the definition file
    for def_e in _errors:
        if not def_e.__seen_in_grep:
            print >> sys.stderr, "Unused Error: CHEROKEE_ERROR_%s" % (def_e.id)
            error_found = True

    return error_found


def check_parameters (dirs):
    known_errors_params = {}
    source_errors_params = {}

    # Known
    for error in _errors:
        tmp = error.title + error.description + error.url_admin + error.debug
        tmp = tmp.replace('%%', '')
        known_errors_params [error.id] = tmp.count('%')

    # Source
    for d in dirs:
        for f in os.listdir(d):
            fullpath = os.path.join (d, f)
            if not fullpath.endswith('.c') or \
               not os.path.isfile(fullpath):
                continue

            # Extract the CHEROKEE_ERROR_*
            content = open (fullpath, 'r').read()

            matches_log = re.findall (r'LOG_[\w_]+ ?' +\
                                      r'\(CHEROKEE_ERROR_([\w_]+)[ ,\n\t\\]*' +\
                                      r'(.*?)\);', content, re.MULTILINE | re.DOTALL)

            matches_errno = re.findall (r'LOG_ERRNO[_S ]*' +\
                                        r'\(.+?,.+?,[ \n\t]*CHEROKEE_ERROR_([\w_]+)[ ,\n\t\\]*' +\
                                        r'(.*?)\);', content, re.MULTILINE | re.DOTALL)

            matches = matches_errno + matches_log
            for match in matches:
                error  = match[0]
                params = match[1].strip()

                # Remove internal sub-parameters (function parameters)
                tmp = params
                while True:
                    internal_params = re.findall(r'(\(.*?\))', tmp)
                    if not internal_params:
                        break
                    for param in internal_params:
                        tmp = tmp.replace(param, '')

                params_num = len (filter (lambda x: len(x), tmp.split(',')))
                source_errors_params[error] = params_num

    # Compare both
    error_found = False

    for error in _errors:
        source_num = source_errors_params[error.id]
        known_num  = known_errors_params[error.id]
        if source_num != known_num:
            print >> sys.stderr, "ERROR: Parameter number mismatch: %s (source %d, definition %d)" % (error.id, source_num, known_num)
            error_found = True

    return error_found

#
# File generation
#

HEADER = """/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2014 Alvaro Lopez Ortega
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/* NOTE: File automatically generated (by error_list.py). */

"""

def generate_C_defines ():
    txt = ''
    max_len = max([len(e.id) for e in _errors])

    for num in range(len(_errors)):
        err = _errors[num]
        pad = " " * (max_len - len(err.id))
        txt += '#define CHEROKEE_ERROR_%s %s %d\n' %(err.id.upper(), pad, num)

    return txt

def generate_C_errors ():
    txt = ''
    txt += 'static const cherokee_error_t __cherokee_errors[] =\n'
    txt += '{\n'
    for num in range(len(_errors)):
        err = _errors[num]

        txt += '  { % 3d, "%s", ' %(num, err.title)

        if err.description:
            txt += '"%s", ' % (err.description)
        else:
            txt += 'NULL, '

        if err.url_admin:
            txt += '"%s", ' % (err.url_admin)
        else:
            txt += 'NULL, '

        if err.debug:
            txt += '"%s", ' % (err.debug)
        else:
            txt += 'NULL, '

        txt += ('false', 'true')[err.show_bt]

        txt += ' },\n'
    txt += '  {  -1, NULL, NULL, NULL, NULL, true }\n'
    txt += '};\n'
    return txt


def main():
    # Check parameters
    error = False

    do_defines = '--defines'    in sys.argv
    do_errors  = '--errors'     in sys.argv
    dont_test  = '--skip-tests' in sys.argv

    if len(sys.argv) >= 1:
        file = sys.argv[-1]
        if file.startswith ('-'):
            error = True
        if not do_defines and not do_errors:
            error = True
    else:
        error = True

    if error:
        print "USAGE:"
        print
        print " * Create the definitions file:"
        print "    %s [--skip-tests] --defines output_file" %(sys.argv[0])
        print
        print " * Create the error list file:"
        print "    %s [--skip-tests] --errors output_file" %(sys.argv[0])
        print
        sys.exit(1)

    # Perform
    read_errors()

    if not dont_test:
        error =  check_source_code (SOURCE_DIRS)
        error |= check_parameters (SOURCE_DIRS)
        if error:
            sys.exit(1)

    if do_defines:
        txt  = HEADER
        txt += generate_C_defines()
        open (file, 'w+').write(txt)

    if do_errors:
        txt  = HEADER
        txt += generate_C_errors()
        open (file, 'w+').write(txt)


if __name__ == '__main__':
    main()
