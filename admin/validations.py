import string
import os.path

def is_boolean (value):
    if value.lower() in ['on', '1', 'true']:
        return '1'
    return '0'

def is_tcp_port (value):
    try:
        tmp = int(value)
    except:
        raise ValueError, 'Port must be a number'
    if tmp < 0 or tmp > 0xFFFF:
        raise ValueError, 'Out of the range (1 to 65535)'
    return value

def is_path (value):
    is_not_empty(value)
    if value[0] == '/':
        return value
    if value[1:3] == ":\\":
        return value
    raise ValueError, 'Malformed path'

def is_dir_formated (value):
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

def is_path_list (value):
    re = []
    for p in value.split(','):
        re.append(is_path(p))
    return reduce(lambda x,y: x+','+y, re)

def is_positive_int (value):
    tmp = int(value)
    if tmp < 0:
        raise ValueError, 'It cannot be negative'
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
        raise ValueError, 'Malformed IPv4'
    for byte in parts:
        try:
            v = int(byte)
        except:
            raise ValueError, 'Malformed IPv4 entry'
        if v < 0 or v > 255:
            raise ValueError, 'IPv4 entry out of range'
    return value

def is_ipv6 (value):
    from socket import inet_pton, AF_INET6
    try: 
        tmp = inet_pton(AF_INET6, value)
    except:
        raise ValueError, 'Malformed IPv6'
    return value
    
def is_local_dir_exists (value, cfg):
    value = is_path (value)

    chroot = cfg.get_val('server!chroot')
    if chroot:
        path = os.path.normpath (chroot + os.path.sep + value)
    else:
        path = value

    if not os.path.exists(path):
        raise ValueError, 'Path does not exit'

    if not os.path.isdir(path):
        raise ValueError, 'Path is not a directory'

    return value

def is_local_file_exists (value, cfg):
    value = is_path (value)

    chroot = cfg.get_val('server!chroot')
    if chroot:
        path = os.path.normpath (chroot + os.path.sep + value)
    else:
        path = value

    if not os.path.exists(path):
        raise ValueError, 'Path does not exit'

    if not os.path.isfile(path):
        raise ValueError, 'Path is not a regular file'

    return value

def parent_is_dir (value, cfg):
    value = is_path (value)

    dirname, filename = os.path.split(value)
    is_local_dir_exists (dirname, cfg)

    return value

def is_safe_id (value):
    for v in value:
        if v not in string.letters + string.digits and \
           v not in "_-.":
           raise ValueError, 'Invalid character '+v
    return value

def is_safe_id_list (value):
    ids = [id.strip() for id in value.split(',')]
    for id in ids:
        is_safe_id (id)
    return ','.join(ids)

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
        raise ValueError, 'Neither a number or an IPv4'

    is_ipv4 (value)

    bin = ''
    for part in value.split("."):
        bin += int2bin(int(part), 8)
        
    zeros_began = False
    for d in bin:
        if d == '1' and zeros_began:
            raise ValueError, 'Invalid mask'
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
        raise ValueError, 'Malformed entry (netmask)'
    
    ip = is_ip (parts[0])
    nm = is_netmask (parts[1])

    return "%s/%s" % (ip, nm)

def is_ip_or_netmask_list (value):
    re = []
    for entry in value.split(','):
        e = entry.strip()
        re.append(is_ip_or_netmask(e))
    return ','.join(re)

def is_not_empty (value):
    if len(value) <= 0:
        raise ValueError, 'Cannot be empty'
    return value

def debug_fail (value):
    raise ValueError, 'Forced failure'
    
def is_regex (value):
    # Can a regular expression be checked?
    return value

def is_http_url (value):
    if not value.startswith('http://'):
        raise ValueError, 'http:// missing'
    return value

def is_url_or_path (value):
    if value.startswith('http://'):
        return is_http_url (value)

    if value.startswith('/'):
        return is_path (value)

    raise ValueError, 'Not a URL, nor a path'


