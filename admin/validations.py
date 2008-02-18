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
    if tmp < 0 or tmp > 0xFFFF - 1:
        raise ValueError, 'Out of the range (1 to 65534)'
    return value

def is_path (value):
    if not value:
        raise ValueError, 'Path cannot be empty'
    if value[0] == '/':
        return value
    if value[1:3] == ":\\":
        return value
    raise ValueError, 'Malformed path'

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
    
def is_local_dir_exists (value):
    value = is_path (value)

    if not os.path.exists(value):
        raise ValueError, 'Path does not exits'

    if not os.path.isdir(value):
        raise ValueError, 'Path is not a directory'

    return value

def is_local_file_exists (value):
    value = is_path (value)

    if not os.path.exists(value):
        raise ValueError, 'Path does not exits'

    if not os.path.isfile(value):
        raise ValueError, 'Path is not a regular file'

    return value

def parent_is_dir (value):
    value = is_path (value)

    dirname, filename = os.path.split(value)
    is_local_dir_exists (dirname)

    return value

def is_safe_id (value):
    for v in value:
        if v not in string.ascii_letters and \
           v not in "_-":
           raise ValueError, 'Invalid character '+v
    return value
