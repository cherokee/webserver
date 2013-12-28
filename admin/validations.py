import CTK
import string
import os.path
import re
from util import split_list, get_real_path

# Conditional Check
# -----------------
# By default the validations are not performed if there is no value to
# check. That usually means that the configuation entry is being
# removed. In case the entry should always be checked, the function
# could define a property 'CHECK_ON_NO_VALUE' to enforce it.

OPTIONAL = 'optional'

def is_number (value):
    try:
        return str(int(value))
    except:
        raise ValueError, _('Malformed number')

def is_float (value):
    try:
        return str(float(value))
    except:
        raise ValueError, _('Malformed float number')

def is_number_gt_0 (value):
    value = is_number(value)
    if int(value) <= 0:
        raise ValueError, _('Must be greater than zero')
    return value

def is_boolean (value):
    if value.lower() in ['on', '1', 'true']:
        return '1'
    return '0'

def is_tcp_port (value):
    try:
        tmp = int(value)
    except:
        raise ValueError, _('Port must be a number')
    if tmp < 0 or tmp > 0xFFFF:
        raise ValueError, _('Out of range (1 to 65535)')
    return value

def is_new_tcp_port (value):
    value    = is_tcp_port (value)
    bindings = CTK.cfg.keys ('server!bind')
    ports    = [CTK.cfg.get_val('server!bind!%s!port'%(x)) for x in bindings]
    if value in ports:
        raise ValueError, _('Port already in use')
    return value

def is_extension (value):
    is_not_empty(value)
    return value

def is_web_path (value):
    is_not_empty(value)
    if value[0] == '/':
        return value
    raise ValueError, _('Malformed path')

def strip_trailing_slashes (value):
    while len(value) > 1 and  value[-1] == '/':
        value = value [:-1]
    return value

def is_path (value):
    is_not_empty(value)
    value = strip_trailing_slashes (value)
    if value[0] == '/':
        return value
    if value[1:3] == ":\\":
        return value
    raise ValueError, _('Malformed path')

def is_list (value):
    tmp = split_list (value)
    if not tmp:
        return ''
    return ','.join(tmp)

def is_dir_formatted (value):
    is_path (value)

    try:
        while len(value) > 1 and \
              value[-1] in ['/', '\\']:
            value = value[:-1]
    except:
        is_not_empty('')

    # Replace double slashes
    while True:
        tmp = value.replace('//','/')
        tmp = tmp.replace ('\\\\', '\\')
        if tmp == value:
            break
        value = tmp

    return value

def is_exec_path (value):
    is_path(value)
    if not os.access (value, os.X_OK):
        raise ValueError, _('It is not executable')
    return value

def is_exec_file (value):
    value = is_exec_path (value)
    if not os.path.isfile(value):
        raise ValueError, _('Path is not a regular file')
    return value

def is_extension_list (value):
    re = []
    for p in split_list(value):
        re.append(is_extension(p))
    return ','.join(re)

def is_path_list (value):
    re = []
    for p in split_list(value):
        re.append(is_path(p))
    return ','.join(re)

def is_positive_int (value):
    tmp = int(value)
    if tmp < 0:
        raise ValueError, _('It cannot be negative')
    return value

def is_ip (value):
    if ':' in value:
        return is_ipv6(value)
    return is_ipv4(value)

def is_netmask (value):
    if ':' in value:
        return is_netmask_ipv6(value)
    return is_netmask_ipv4(value)

def is_ipv4 (value):
    parts = value.split('.')
    if len(parts) != 4:
        raise ValueError, _('Malformed IPv4')
    for byte in parts:
        try:
            v = int(byte)
        except:
            raise ValueError, _('Malformed IPv4 entry')
        if v < 0 or v > 255:
            raise ValueError, _('IPv4 entry out of range')
    return value

def is_ipv6 (value):
    from socket import inet_pton, AF_INET6
    try:
        tmp = inet_pton(AF_INET6, value)
    except:
        raise ValueError, _('Malformed IPv6')
    return value

def is_chroot_dir_exists (value):
    return is_local_dir_exists (value, nochroot=True)

def is_local_dir_exists (value, nochroot=False):
    value = is_path (value)
    path  = get_real_path (value, nochroot)

    if not os.path.exists(path):
        raise ValueError, _('Path does not exist')

    if not os.path.isdir(path):
        raise ValueError, _('Path is not a directory')

    return value

def is_local_file_exists (value, nochroot=False):
    value = is_path (value)
    path  = get_real_path (value, nochroot)

    if not os.path.exists(path):
        raise ValueError, _('Path does not exist')

    if not os.path.isfile(path):
        raise ValueError, _('Path is not a regular file')

    return value

def parent_is_dir (value, nochroot=False):
    value = is_path (value)

    dirname, filename = os.path.split(value)
    is_local_dir_exists (dirname, nochroot)

    return value

def can_create_file (value, nochroot=False):
    value = parent_is_dir (value)

    path = get_real_path (value, nochroot)
    if os.path.isdir(path):
        raise ValueError, _('Path is a directory')

    return value

def is_safe_id (value):
    for v in value:
        if v not in string.letters and \
           v not in string.digits  and \
           v not in "_-.":
            raise ValueError, _('Invalid char. Accepts: letters, digits, _, - and .')
    return value

def is_safe_id_list (value):
    ids = []
    for id in split_list (value):
        ids.append(is_safe_id (id))
    return ','.join(ids)

def is_header_name (value):
    return is_safe_id (value)

def int2bin(n, count=24):
    """returns the binary of integer n, using count number of digits"""
    return "".join([str((n >> y) & 1) for y in range(count-1, -1, -1)])

def is_netmask_ipv4 (value):
    bits = None
    try:
        bits = int(value)
        if bits > 0 and bits <= 32:
            return value
    except:
        pass

    if not "." in value:
        raise ValueError, _('Neither a number or an IPv4')

    is_ipv4 (value)

    bin = ''
    for part in value.split("."):
        bin += int2bin(int(part), 8)

    zeros_began = False
    for d in bin:
        if d == '1' and zeros_began:
            raise ValueError, _('Invalid mask')
        if d == '0' and not zeros_began:
            zeros_began = True

    return value

def is_netmask_ipv6 (value):
    bits = None
    try:
        bits = int(value)
        if bits > 0 and bits <= 128:
            return value
    except:
        pass

    # TODO
    return value

def is_ip_or_netmask (value):
    if not '/' in value:
        return is_ip (value)

    parts = value.split('/')
    if len(parts) != 2:
        raise ValueError, _('Malformed entry (netmask)')

    if ':' in value:
        ip = is_ipv6 (parts[0])
        nm = is_netmask_ipv6 (parts[1])
    else:
        ip = is_ipv4 (parts[0])
        nm = is_netmask_ipv4 (parts[1])

    return "%s/%s" % (ip, nm)

def is_ip_list (value):
    re = []
    for e in split_list(value):
        re.append(is_ip(e))
    return ','.join(re)

def is_ip_or_netmask_list (value):
    re = []
    for e in split_list(value):
        re.append(is_ip_or_netmask(e))
    return ','.join(re)

def is_not_empty (value):
    if len(value) <= 0:
        raise ValueError, _('Cannot be empty')
    return value
is_not_empty.CHECK_ON_NO_VALUE = True

def debug_fail (value):
    raise ValueError, _('Forced failure')

def is_regex (value):
    # Can a regular expression be checked?
    return value

def is_http_url (value):
    if not value.startswith('http://'):
        raise ValueError, _('http:// missing')
    return value

def is_url_or_path (value):
    if value.startswith('http://'):
        return is_http_url (value)

    if value.startswith('/'):
        return is_path (value)

    raise ValueError, 'Not a URL, nor a path'

def is_dev_null_or_local_dir_exists (value, nochroot=False):
    if value == '/dev/null':
        return value
    return is_local_dir_exists (value, nochroot)

def is_information_source (value):
    # /unix/path
    if value[0] == '/':
        return value

    # http://host/path -> host/path
    if "://" in value:
        value = value.split("://")[1]

    # host/path -> host
    if '/' in value:
        return value.split('/')[0]

    return value

def is_time (value):
    # The m, h, d and w suffixes are allowed for minutes, hours, days,
    # and weeks. Eg: 2d.
    for c in value:
        if c not in ".0123456789mhdw":
            raise ValueError, _('Time value contain invalid values')
    for c in ".mhdw":
        if value.count(c) > 1:
            raise ValueError, _('Malformed time')

    return value

def is_new_vserver_nick (value):
    value = is_not_empty(value)
    for h in CTK.cfg.keys('vserver'):
        if value == CTK.cfg.get_val('vserver!%s!nick'%(h)):
            raise ValueError, _('Virtual Server nick name is already being used.')
    return value


def is_safe_mime_type (mime):
    mimes = CTK.cfg.keys ('mime')
    if mime in mimes:
        raise ValueError, _('Already in use')
    return mime


def is_safe_information_source_host (value, exclude = None):
    host  = is_information_source (value)
    keys  = ['source!%s!host'%(key) for key in CTK.cfg.keys ('source')]
    hosts = [CTK.cfg.get_val(key) for key in keys if key != exclude]

    if host in hosts:
        raise ValueError, _('Already in use')
    return host


def is_safe_information_source_nick (value, exclude = None):
    nick  = is_not_empty(value)
    keys  = ['source!%s!nick'%(key) for key in CTK.cfg.keys ('source')]
    nicks = [CTK.cfg.get_val(key) for key in keys if key != exclude]

    if nick in nicks:
        raise ValueError, _('Already in use')
    return nick


def is_safe_cfgval (key, cfg_str, new, safe):
    new  = list(set(split_list (new))) # remove list duplicates
    keys = CTK.cfg.keys (key)
    cfg  = [CTK.cfg.get_val(cfg_str % key) for key in keys]
    old  = [x for sublist in map(split_list, cfg) for x in sublist]

    old.sort()
    dupes = list (set(old) & set(new))
    if safe: # Do not worry about safe values
        safe  = split_list (safe)
        dupes = list (set(dupes) - set(safe))

    if dupes:
        raise ValueError, '%s: %s' %(_('Already in use'), ', '.join(dupes))

    return ','.join(new)


def is_safe_mime_exts (new, safe = None):
    return is_safe_cfgval ('mime', 'mime!%s!extensions', new, safe)


def is_safe_icons_suffix (new, safe = None):
    return is_safe_cfgval ('icons!suffix', 'icons!suffix!%s', new, safe)


def is_safe_icons_file (new, safe = None):
    return is_safe_cfgval ('icons!file', 'icons!file!%s', new, safe)

def is_positive_int_4_multiple (value):
    tmp = is_positive_int (value)
    num = int(tmp)
    while num % 4 != 0:
        num += 1
    return str(num)

def is_email (value):
    is_not_empty(value)

    if re.match (r"^\S+@\S+\.\S+$", value) == None:
        raise ValueError, _("Please insert a valid email")

    return value

def has_no_quotes (value):
    if "'" in value:
        raise ValueError, _("Cannot contain the quote (') character")
    return value

def has_no_double_quotes (value):
    if '"' in value:
        raise ValueError, _('Cannot contain the double quote (") character')
    return value

def is_valid_certkey(value):
    is_local_file_exists(value)

    cert_data = open(value, 'r').read()
    if "ENCRYPTED" in cert_data:
        raise UserWarning, (_('The private key contains a passphrase which has to be entered manually after launching the webserver.'), value)
    return value
